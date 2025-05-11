#include "multi_pass_renderpass.hpp"

#include "../image.hpp"
#include <vke/util.hpp>

#include "../window/surface.hpp"
#include "../window/window.hpp"

#include "../commandbuffer.hpp"

#include "renderpass_builder.hpp"

namespace vke {

MultiPassRenderPass::MultiPassRenderPass(RenderPassBuilder* builder, u32 width, u32 height) {
    m_width  = width;
    m_height = height;

    uint32_t subpass_index = 0;

    m_clear_values     = builder->m_clear_values;
    m_attachment_infos = builder->m_attachment_infos;
    m_renderpass       = builder->create_vk_renderpass();

    m_attachment_layer_count      = builder->m_layer_count;
    m_frame_buffer_instance_count = builder->m_layer_count;

    m_subpasses = vke::map_vec(builder->m_subpass_info, [&](const impl::SubpassInfo& info) {
        SubpassDetails d;
        d.renderpass                = this;
        d.render_target_description = {
            .color_attachments = vke::map_vec2small_vec<0>(info.color_attachments, [&](VkAttachmentReference c) { return m_attachment_infos[c.attachment].description.format; }),
            .depth_attachment  = m_attachment_infos[info.depth_attachment->attachment].description.format,
        };
        d.subpass_index = subpass_index++;
        return d;
    });

    create_attachments();
    create_framebuffers();
}

void MultiPassRenderPass::create_attachments() {
    m_attachments = map_vec(m_attachment_infos, [this](const impl::AttachmentInfo& info) {
        if (info.is_surface_attachment) return Attachment{};

        VkImageUsageFlags flags = is_depth_format(info.description.format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (info.is_sampled) {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (info.is_input_attachment) {
            flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }

        return Attachment{
            .image = static_cast<std::unique_ptr<vke::IImageView>>(std::make_unique<vke::Image>(ImageArgs{
                .format      = info.description.format,
                .usage_flags = flags,
                .width       = width(),
                .height      = height(),
                .layers      = m_attachment_layer_count,
            })),
        };
    });
}

void MultiPassRenderPass::create_framebuffers() {
    assert(!(m_attachment_layer_count > 1 && m_has_surface_attachment) && "multi layers and surface attachments are not supported for MultiPassRenderPass");
    assert(m_rendered_layer_count == 1 && "m_rendered_layer_count of only 1 is supported");

    if (!m_framebuffers.empty()) {
        LOG_WARNING("creating framebuffers while m_framebuffers isn't empty. clearing frame buffers!");
        m_framebuffers.clear();
    }

    for (u32 i = 0; i < m_attachment_layer_count; i++) {

        auto vke_views = vke ::map_vec2small_vec(m_attachments, [&](Attachment& att) -> vke::RCResource<IImageView> {
            assert(att.image != nullptr && "attachment image cannot be null");
            if (i == 0) return att.image;

            return dynamic_cast<vke::Image*>(att.image.get())->create_subview(SubViewArgs{
                .base_layer  = i,
                .layer_count = 1,
                .view_type   = VK_IMAGE_VIEW_TYPE_2D,
            });
        });

        auto attachment_views = vke ::map_vec2small_vec(vke_views, [](const RCResource<IImageView>& view) { return view->view(); });

        VkFramebufferCreateInfo fb_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = m_renderpass,
            .attachmentCount = static_cast<uint32_t>(attachment_views.size()),
            .pAttachments    = attachment_views.data(),
            .width           = m_width,
            .height          = m_height,
            .layers          = 1,
        };

        if (m_has_surface_attachment) {
            auto surface                = m_window->surface();
            auto& swapchain_image_views = surface->get_swapchain_image_views();

            for (u32 i; i < swapchain_image_views.size(); i++) {
                attachment_views[m_surface_attachment_index] = swapchain_image_views[i];

                VkFramebuffer framebuffer;
                VK_CHECK(vkCreateFramebuffer(device(), &fb_info, nullptr, &framebuffer));

                m_framebuffers.push_back(std::make_unique<impl::Framebuffer>(framebuffer));
            }
        } else {
            VkFramebuffer framebuffer;
            VK_CHECK(vkCreateFramebuffer(device(), &fb_info, nullptr, &framebuffer));

            m_framebuffers.push_back(std::make_unique<impl::Framebuffer>(framebuffer));
        }

        for (auto& fb : m_framebuffers) {
            for (auto& view : vke_views) {
                fb->add_execution_dependency(view->get_reference());
            }
        }
    }
}

void MultiPassRenderPass::destroy_framebuffers() {
    // for (auto& fb : m_framebuffers) {
    //     vkDestroyFramebuffer(device(), fb, nullptr);
    //     fb = nullptr;
    // }
    m_framebuffers.resize(0);
}

VkFramebuffer MultiPassRenderPass::next_framebuffer() {
    u32 base_index = m_active_frame_buffer_instance;

    if (m_has_surface_attachment) {
        assert(m_window);
        auto* surface = m_window->surface();
        assert(surface);
        base_index *= surface->get_swapchain_images().size();
        return m_framebuffers[base_index + surface->get_swapchain_image_index()]->handle();
    }

    return m_framebuffers[base_index]->handle();
}

MultiPassRenderPass::~MultiPassRenderPass() {
    destroy_framebuffers();
}

void MultiPassRenderPass::begin(CommandBuffer& cmd) {
    if (m_has_surface_attachment) {
        m_window->surface()->prepare();
    }

    cmd.add_execution_dependency(m_framebuffers[m_window ? m_window->surface()->get_swapchain_image_index() : 0]->get_reference());

    Renderpass::begin(cmd);
}

IImageView* MultiPassRenderPass::get_attachment_view(u32 index) { return m_attachments[index].image.get(); }

void MultiPassRenderPass::resize(CommandBuffer& cmd, u32 width, u32 height) {
    LOG_INFO("resizing multi render pass to (%d,%d)", width, height);

    m_attachments.clear();
    destroy_framebuffers();

    m_width  = width;
    m_height = height;

    create_attachments();
    create_framebuffers();
};

void MultiPassRenderPass::set_active_frame_buffer_instance(u32 i)  {
    assert(i < m_frame_buffer_instance_count && "active instance must be in the range of frame buffer instance count!");

    m_active_frame_buffer_instance = i;
}
} // namespace vke