#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

namespace vke {

class VulkanContext;

struct SurfaceData {
};

struct SurfaceArgs {
};

struct SwapChainData {
	VkSwapchainKHR swapchain;
	VkFormat format;
	VkExtent2D extend;
	std::vector<VkImage> images;
	std::vector<VkImageView> views;
};

struct SwapChainArgs {
    VkSwapchainKHR old_swapchain = nullptr;
    uint32_t width, height;
    VkPresentModeKHR present_mode;
	VkSurfaceKHR surface;
};

SwapChainData create_swapchain(VulkanContext* ctx, const SwapChainArgs& swapchain_args);

} // namespace vke