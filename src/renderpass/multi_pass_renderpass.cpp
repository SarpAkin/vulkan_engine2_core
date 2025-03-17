#include "multi_pass_renderpass.hpp"

#include <vke/util.hpp>
#include "../image.hpp"

#include "../window/window.hpp"
#include "../window/surface.hpp"

#include "renderpass_builder.hpp"

namespace vke {

MultiPassRenderPass::MultiPassRenderPass(RenderPassBuilder* builder, u32 width, u32 height)  {
    m_width  = width;
    m_height = height;

    m_clear_values     = builder->m_clear_values;
    m_attachment_infos = builder->m_attachment_infos;
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
    return m_framebuffers[m_window->surface()->get_swapchain_image_index()];
}

MultiPassRenderPass::~MultiPassRenderPass() {
    destroy_framebuffers();
}

void MultiPassRenderPass::begin(CommandBuffer& cmd) {
    if(m_has_surface_attachment){
        m_window->surface()->prepare();
    }

    Renderpass::begin(cmd);
}

} // namespace vke