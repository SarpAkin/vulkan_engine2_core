#include "multi_pass_renderpass.hpp"

#include "../image.hpp"
#include <vke/util.hpp>

#include "../window/surface.hpp"
#include "../window/window.hpp"

#include "../commandbuffer.hpp"
#include "../vulkan_context.hpp"

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

    m_attachments = map_vec(m_attachment_infos, [&](const impl::AttachmentInfo& info) {
        if (info.is_surface_attachment) return Attachment{};

        bool depth_format = is_depth_format(info.description.format);

        VkImageUsageFlags flags = depth_format ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (info.is_sampled) {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (info.is_input_attachment) {
            flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }

        auto image = std::make_unique<vke::Image>(ImageArgs{
            .format      = info.description.format,
            .usage_flags = flags,
            .width       = width(),
            .height      = height(),
            .layers      = m_attachment_layer_count,
        });

        return Attachment{
            .image = static_cast<std::unique_ptr<vke::IImageView>>(std::move(image)),
        };
    });

    VulkanContext::get_context()->immediate_submit([&](vke::CommandBuffer& cmd) {
        clear_sampled_attachments(&cmd);
    });
}

void MultiPassRenderPass::clear_sampled_attachments(CommandBuffer* cmd) {
    vke::SmallVec<VkImageMemoryBarrier, 6> barriers;

    for (int i = 0; i < m_attachments.size(); i++) {
        auto& info       = m_attachment_infos[i];
        auto& attachment = m_attachments[i];
        auto* image      = dynamic_cast<vke::Image*>(attachment.image.get());

        if (!info.is_sampled) continue;

        barriers.push_back({
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask    = VK_ACCESS_NONE,
            .dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image            = image->handle(),
            .subresourceRange = image->get_subresource_range(),
        });
    }

    cmd->pipeline_barrier({
        .src_stage_mask        = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .image_memory_barriers = barriers,
    });

    barriers.clear();

    for (int i = 0; i < m_attachments.size(); i++) {
        auto& info       = m_attachment_infos[i];
        auto& attachment = m_attachments[i];
        auto* image      = dynamic_cast<vke::Image*>(attachment.image.get());

        if (!info.is_sampled) continue;

        bool depth_format = is_depth_format(info.description.format);

        VkClearValue cvs[1];
        if (depth_format) {
            cvs[0] = VkClearValue{
                .depthStencil = {1.0, 0},
            };
        } else {
            cvs[0] = VkClearValue{
                .color = {0, 0, 0, 0},
            };
        }

        VkImageSubresourceRange sr[1] = {image->get_subresource_range()};

        cmd->clear_image(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cvs, sr);

        VkAccessFlags mask = (depth_format ? (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT) : (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));

        barriers.push_back({
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask    = VK_ACCESS_MEMORY_READ_BIT,
            .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image            = image->handle(),
            .subresourceRange = sr[0],
        });
    }

    cmd->pipeline_barrier({
        .src_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .image_memory_barriers = barriers,
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

void MultiPassRenderPass::set_active_frame_buffer_instance(u32 i) {
    assert(i < m_frame_buffer_instance_count && "active instance must be in the range of frame buffer instance count!");

    m_active_frame_buffer_instance = i;
}
} // namespace vke