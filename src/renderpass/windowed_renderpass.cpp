#include "windowed_renderpass.hpp"

#include <vulkan/vulkan_core.h>

#include <vke/util.hpp>

#include "../common.hpp"
#include "../fwd.hpp"

#include "../commandbuffer.hpp"
#include "../image.hpp"

#include "../window/window.hpp"

namespace vke {

WindowRenderPass::WindowRenderPass(Window* window, bool has_depth) {
    m_width       = window->width();
    m_height      = window->height();
    m_window      = window;
    // m_target_size = window;

    m_clear_values = {VkClearValue{.color = VkClearColorValue{0.f, 0.f, 1.f, 0.f}}};
    if (has_depth) m_clear_values.push_back({VkClearValue{.depthStencil = VkClearDepthStencilValue{.depth = 1.0}}});

    if (has_depth) {
        create_depth_buffer();
    }

    init_renderpass();
    create_framebuffers();

    SubpassDetails sp;
    sp.render_target_description = {
        .color_attachments = {m_window->surface()->get_swapchain_image_format()}, 
    };

    sp.renderpass        = this;
    sp.subpass_index     = 0;

    m_subpasses.push_back(std::move(sp));
}

void WindowRenderPass::init_renderpass() {
    auto surface = m_window->surface();
    surface->init_swapchain();

    std::vector<VkAttachmentDescription> attachments{
        VkAttachmentDescription{
            .format        = surface->get_swapchain_image_format(),
            .samples       = VK_SAMPLE_COUNT_1_BIT,
            .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
    };

    if (m_depth) {
        attachments.push_back(VkAttachmentDescription{
            .format        = m_depth->format(),
            .samples       = VK_SAMPLE_COUNT_1_BIT,
            .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        });
    }

    VkAttachmentReference attachment_references[] = {
        VkAttachmentReference{
            .attachment = 0,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };

    VkAttachmentReference depth_attachment{
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpasses[] = {
        VkSubpassDescription{
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = ARRAY_LEN(attachment_references),
            .pColorAttachments       = attachment_references,
            .pDepthStencilAttachment = m_depth ? &depth_attachment : nullptr,
        },
    };

    VkRenderPassCreateInfo info{
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<u32>(attachments.size()),
        .pAttachments    = attachments.data(),
        .subpassCount    = ARRAY_LEN(subpasses),
        .pSubpasses      = subpasses,
    };

    VK_CHECK(vkCreateRenderPass(device(), &info, nullptr, &m_renderpass));
}

void WindowRenderPass::create_framebuffers() {
    auto surface            = m_window->surface();
    m_swapchain_image_views = surface->get_swapchain_image_views();

    VkImageView attachment_views[2] = {nullptr};

    m_framebuffers.resize(m_swapchain_image_views.size());

    for (int i = 0; i < m_swapchain_image_views.size(); ++i) {
        attachment_views[0] = m_swapchain_image_views[i];
        if (m_depth) attachment_views[1] = m_depth->view();

        VkFramebufferCreateInfo fb_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = m_renderpass,
            .attachmentCount = static_cast<u32>(m_depth ? 2 : 1),
            .pAttachments    = attachment_views,
            .width           = width(),
            .height          = height(),
            .layers          = 1,
        };

        VkFramebuffer fb;
        VK_CHECK(vkCreateFramebuffer(device(), &fb_info, nullptr, &fb));

        m_framebuffers[i] = std::make_unique<impl::Framebuffer>(fb);
    }
}

void WindowRenderPass::destroy_framebuffers() {
    m_framebuffers.resize(0);
}

VkFramebuffer WindowRenderPass::next_framebuffer() {
    return m_framebuffers[m_window->surface()->get_swapchain_image_index()]->handle();
}

WindowRenderPass::~WindowRenderPass() {
    destroy_framebuffers();
}

void WindowRenderPass::begin(CommandBuffer& cmd) {
    auto surface = m_window->surface();
    if (m_swapchain_image_views != surface->get_swapchain_image_views()) {
        resize(cmd, surface->width(), surface->height());
    }

    cmd.add_execution_dependency(m_framebuffers[m_window->surface()->get_swapchain_image_index()]->get_reference());
    if (m_depth) {
        cmd.add_execution_dependency(m_depth->get_reference());
    }

    Renderpass::begin(cmd);
}

void WindowRenderPass::resize(CommandBuffer& cmd, u32 width, u32 height) {
    destroy_framebuffers();
    // m_window->surface()->recrate_swapchain();
    m_width  = width;
    m_height = height;

    if (m_depth) create_depth_buffer();

    create_framebuffers();
}

void WindowRenderPass::add_framebuffers_as_dependency(vke::CommandBuffer& cmd) {
    for (auto& fb : m_framebuffers) {
        cmd.add_execution_dependency(fb->get_reference());
    }
}
void WindowRenderPass::create_depth_buffer() {
    m_depth = std::make_unique<vke::Image>(ImageArgs{
        .format      = VK_FORMAT_D16_UNORM,
        .usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .width       = width(),
        .height      = height(),
    });
}
} // namespace vke