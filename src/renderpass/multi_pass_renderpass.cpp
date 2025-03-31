#include "multi_pass_renderpass.hpp"

#include "../image.hpp"
#include <vke/util.hpp>

#include "../window/surface.hpp"
#include "../window/window.hpp"

#include "renderpass_builder.hpp"

namespace vke {

MultiPassRenderPass::MultiPassRenderPass(RenderPassBuilder* builder, u32 width, u32 height) {
    m_width  = width;
    m_height = height;

    uint32_t subpass_index = 0;

    m_clear_values     = builder->m_clear_values;
    m_attachment_infos = builder->m_attachment_infos;
    m_renderpass       = builder->create_vk_renderpass();

    m_subpasses = vke::map_vec(builder->m_subpass_info, [&](const impl::SubpassInfo& info) {
        SubpassDetails d;
        d.renderpass        = this;
        d.depth_format      = m_attachment_infos[info.depth_attachment->attachment].description.format;
        d.color_attachments = vke::map_vec(info.color_attachments, [&](VkAttachmentReference c) {
            return m_attachment_infos[c.attachment].description.format;
        });
        d.subpass_index     = subpass_index++;
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
            .image = std::make_unique<vke::Image>(ImageArgs{
                .format      = info.description.format,
                .usage_flags = flags,
                .width       = width(),
                .height      = height(),
                .layers      = 1,
            }),
        };
    });
}

void MultiPassRenderPass::create_framebuffers() {
    auto attachment_views = MAP_VEC_ALLOCA(m_attachments, [](const Attachment& att) {
        return att.image != nullptr ? att.image->view() : nullptr;
    });

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

        m_framebuffers.resize(swapchain_image_views.size());

        for (u32 i; i < swapchain_image_views.size(); i++) {
            attachment_views[m_surface_attachment_index] = swapchain_image_views[i];
            VK_CHECK(vkCreateFramebuffer(device(), &fb_info, nullptr, &m_framebuffers[i]));
        }
    } else {
        m_framebuffers.resize(1);
        VK_CHECK(vkCreateFramebuffer(device(), &fb_info, nullptr, &m_framebuffers[0]));
    }
}

void MultiPassRenderPass::destroy_framebuffers() {
    for (auto& fb : m_framebuffers) {
        vkDestroyFramebuffer(device(), fb, nullptr);
        fb = nullptr;
    }
    m_framebuffers.resize(0);
}

VkFramebuffer MultiPassRenderPass::next_framebuffer() {
    return m_window ? m_framebuffers[m_window->surface()->get_swapchain_image_index()] : m_framebuffers[0];
}

MultiPassRenderPass::~MultiPassRenderPass() {
    destroy_framebuffers();
}

void MultiPassRenderPass::begin(CommandBuffer& cmd) {
    if (m_has_surface_attachment) {
        m_window->surface()->prepare();
    }

    Renderpass::begin(cmd);
}

IImageView* MultiPassRenderPass::get_attachment_view(u32 index) { return m_attachments[index].image.get(); }
} // namespace vke