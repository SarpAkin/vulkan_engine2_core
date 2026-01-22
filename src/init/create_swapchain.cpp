#include "create_swapchain.hpp"

#include "../util/util.hpp"
#include "../vkutil.hpp"
#include "../vulkan_context.hpp"

namespace vke {

std::vector<VkSurfaceFormatKHR> get_swapchain_formats(VulkanContext* ctx, VkSurfaceKHR surface) {
    auto& dt = ctx->get_dispatch_table();

    uint32_t format_count;
    VK_CHECK(dt.vkGetPhysicalDeviceSurfaceFormatsKHR(
        ctx->get_physical_device(), surface, &format_count, nullptr) //
    );

    auto formats = std::vector<VkSurfaceFormatKHR>(format_count);
    VK_CHECK(dt.vkGetPhysicalDeviceSurfaceFormatsKHR(
        ctx->get_physical_device(), surface, &format_count, formats.data()) //
    );

    return formats;
}

VkSurfaceFormatKHR choose_swap_surface_format(std::span<const VkSurfaceFormatKHR> formats) {
    assert(formats.size() > 0);

    return formats[0];
}

SwapChainData create_swapchain(VulkanContext* ctx, const SwapChainArgs& swapchain_args) {
    uint32_t graphicsFamily = ctx->get_graphics_queue_family();
    uint32_t presentFamily  = ctx->get_present_queue_family();
    auto availableFormats   = get_swapchain_formats(ctx, swapchain_args.surface);
    auto dt                 = ctx->get_dispatch_table();
    auto device             = ctx->get_device();

    // 1. Query capabilities (Assuming physicalDevice and surface are accessible)
    VkSurfaceCapabilitiesKHR capabilities;
    VK_CHECK(dt.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->get_physical_device(),
        swapchain_args.surface, &capabilities);)

    // 2. Select Surface Format (e.g., B8G8R8A8_SRGB)
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(availableFormats);

    // 3. Set Extent based on SwapChainArgs
    VkExtent2D extent = {swapchain_args.width, swapchain_args.height};
    // Clamp to surface capabilities
    extent.width  = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    // 4. Determine image count
    uint32_t image_count = capabilities.minImageCount + 1;
    // if the maxImageCount is 0 it is unlimited
    if (capabilities.maxImageCount != 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    // 5. Fill the Create Info
    VkSwapchainCreateInfoKHR createInfo{
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = swapchain_args.surface,
        .minImageCount    = image_count,
        .imageFormat      = surface_format.format,
        .imageColorSpace  = surface_format.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform     = capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = swapchain_args.present_mode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = swapchain_args.old_swapchain,
    };

    // Handle Queue Sharing Mode
    uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};
    if (graphicsFamily != presentFamily) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // 6. Create the Swapchain

    VkSwapchainKHR swapchain;
    VK_CHECK(dt.vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain))

    // 7. Retrieve handles for the swapchain images
    std::vector<VkImage> sc_images;
    dt.vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    sc_images.resize(image_count);
    dt.vkGetSwapchainImagesKHR(device, swapchain, &image_count, sc_images.data());

    std::vector<VkImageView> sc_image_views = map_vec(sc_images, [&](VkImage image) {
        VkImageViewCreateInfo view_ci = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = image,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = surface_format.format,
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };

        VkImageView view;
        VK_CHECK(dt.vkCreateImageView(device, &view_ci, nullptr, &view));
        return view;
    });

    return SwapChainData{
        .swapchain = swapchain,
        .format    = surface_format.format,
        .extend    = extent,
        .images    = std::move(sc_images),
        .views     = std::move(sc_image_views),
    };
}
} // namespace vke