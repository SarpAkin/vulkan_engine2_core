#include "vkutil.hpp"

#include <string>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>



namespace vke {
// namespace fs = std::filesystem;

std::string vk_result_string(VkResult res) {
    return vk::to_string(vk::Result(res));
}

bool is_depth_format(VkFormat format) {
    const static VkFormat depth_formats[] = {
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    for (auto df : depth_formats)
        if (df == format) return true;

    return false;
}

}