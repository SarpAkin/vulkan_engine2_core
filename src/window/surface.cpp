#include "surface.hpp"

#include <VkBootstrap.h>
#include <cassert>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "../vulkan_context.hpp"

#include "../semaphore.hpp"
#include "../util/util.hpp"
#include "../vkutil.hpp"
#include "window.hpp"

namespace vke {
void Surface::init_swapchain() {
    vkb::Swapchain vkb_swapchain =
        vkb::SwapchainBuilder(vke::VulkanContext::get_context()->get_physical_device(), device(), m_surface)
            .use_default_format_selection()
            // use vsync present mode
            .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
            .set_desired_extent(m_window->width(), m_window->height())
            .set_old_swapchain(m_swapchain)
            .build()
            .value();

    if (m_swapchain) {
        destroy_swapchain();
    }

    m_width  = vkb_swapchain.extent.width;
    m_height = vkb_swapchain.extent.height;

    m_swapchain_image_format = vkb_swapchain.image_format;
    m_swapchain              = vkb_swapchain.swapchain;
    m_swapchain_image_views  = vkb_swapchain.get_image_views().value();
    m_swapchain_images       = vkb_swapchain.get_images().value();

    int extra_semaphore_count = 3;
    m_prepare_semaphores.resize(m_swapchain_images.size() + extra_semaphore_count);
    m_wait_semaphores.resize(m_swapchain_images.size() + extra_semaphore_count);
}

Surface::~Surface() {
    printf("Surface cleanup\n");

    if (m_swapchain) {
        destroy_swapchain();
    }

    vkDestroySurfaceKHR(vke::VulkanContext::get_context()->get_instance(), m_surface, nullptr);
}

void Surface::destroy_swapchain() {
    vkDestroySwapchainKHR(device(), m_swapchain, nullptr);

    // destroy swapchain resources
    for (auto& iv : m_swapchain_image_views) {
        vkDestroyImageView(device(), iv, nullptr);
    }
}

VkAttachmentDescription Surface::get_color_attachment() const {
    // the renderpass will use this color attachment.
    VkAttachmentDescription color_attachment = {};
    // the attachment will have the format needed by the swapchain
    color_attachment.format = m_swapchain_image_format;
    // 1 sample, we won't be doing MSAA
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // we Clear when this attachment is loaded
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // we keep the attachment stored when the renderpass ends
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // we don't care about stencil
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // we don't know or care about the starting layout of the attachment
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // after the renderpass ends, the image has to be on a layout ready for display
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    return color_attachment;
}

bool Surface::prepare(u64 time_out) {
    assert(m_current_prepare_semaphore == nullptr && "called prepare 2 times before calling present!");
    assert(m_swapchain_image_index < m_wait_semaphores.size());

    auto& prepare_semaphore = m_prepare_semaphores[m_frame_index];
    auto& wait_semaphore    = m_wait_semaphores[m_frame_index];
    if (prepare_semaphore == nullptr) {
        prepare_semaphore = std::make_unique<Semaphore>(); // lazily initialize semaphore since during surface creation device doesn't exist.
        wait_semaphore    = std::make_unique<Semaphore>();
    }

    auto result = vkAcquireNextImageKHR(device(), m_swapchain, time_out, prepare_semaphore->handle(), nullptr, &m_swapchain_image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;
    } else {
        VK_CHECK(result);
    }

    m_current_prepare_semaphore = prepare_semaphore.get();
    m_current_wait_semaphore    = wait_semaphore.get();

    m_frame_index = (m_frame_index + 1) % m_wait_semaphores.size();

    return true;
}

bool Surface::present() {
    VkSemaphore wait_semaphores[] = {m_current_wait_semaphore->handle()};

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,

        .waitSemaphoreCount = ARRAY_LEN(wait_semaphores),
        .pWaitSemaphores    = wait_semaphores,

        .swapchainCount = 1,
        .pSwapchains    = &m_swapchain,

        .pImageIndices = &m_swapchain_image_index,
    };

    m_current_prepare_semaphore = nullptr;
    auto result                 = vkQueuePresentKHR(VulkanContext::get_context()->get_graphics_queue(), &presentInfo);

    if (result == VK_SUCCESS) return true;
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return false;

    VK_CHECK(result);
    return false;
}

u32 Surface::height() const {
    return m_height;
}
u32 Surface::width() const {
    return m_width;
}
void Surface::recrate_swapchain() {
    printf("recrating swapchain! window size: (%d,%d)\n", width(), height());
    init_swapchain();
}

Surface::Surface(VkSurfaceKHR surface, Window* window) {
    m_surface = surface;
    m_window  = window;
    init_swapchain();
}
} // namespace vke