#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include <vke/fwd.hpp>

#include "../semaphore.hpp"
#include "../vk_resource.hpp"

namespace vke {

class Window;

class IRenderTargetSize {
public:
    virtual u32 width() const  = 0;
    virtual u32 height() const = 0;
    VkExtent2D extend() const { return VkExtent2D{.width = width(), .height = height()}; }
};

class Surface : public Resource, public IRenderTargetSize {
public: // IRenderTargetSize
    u32 width() const override;
    u32 height() const override;

public: // getters
    VkSurfaceKHR get_surface() const { return m_surface; }
    VkFormat get_swapchain_image_format() const { return m_swapchain_image_format; }
    VkSwapchainKHR get_swapchain() const { return m_swapchain; }
    const std::vector<VkImage>& get_swapchain_images() const { return m_swapchain_images; }
    const std::vector<VkImageView>& get_swapchain_image_views() const { return m_swapchain_image_views; }
    VkAttachmentDescription get_color_attachment() const;

    Surface(VkSurfaceKHR surface, Window* window) {
        m_surface = surface;
        m_window  = window;
    }
    ~Surface();

    // must be called after device initialization
    void init_swapchain();

    bool prepare(u64 time_out = UINT64_MAX);
    bool present();

    void recrate_swapchain();

    u32 get_swapchain_image_index() const { return m_swapchain_image_index; }

    Semaphore* get_prepare_semaphore() const { return m_current_prepare_semaphore; } // returns null if prepare isn't called that frame
    Semaphore* get_wait_semaphore() const { return m_current_wait_semaphore; }

private:
    void destroy_swapchain();

private:
    Window* m_window;
    VkSurfaceKHR m_surface;
    VkFormat m_swapchain_image_format;
    VkSwapchainKHR m_swapchain = nullptr;
    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;

    u32 m_swapchain_image_index = 0;
    u32 m_frame_index           = 0;
    u32 m_width = 0,m_height = 0;

    // prepare semaphore is signalled after surface preparetion, wait semapore is used to wait for present
    std::vector<std::unique_ptr<Semaphore>> m_prepare_semaphores, m_wait_semaphores;

    Semaphore* m_current_prepare_semaphore = nullptr;
    Semaphore* m_current_wait_semaphore    = nullptr;
};

} // namespace vke