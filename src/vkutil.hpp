#pragma once

#include <cassert>
#include <cstdio>
#include <string>

#include <vulkan/vulkan_core.h>

namespace vke {
std::string vk_result_string(VkResult res);

bool is_depth_format(VkFormat format);


} // namespace vke

#define VK_CHECK(x)                                                                    \
    {                                                                                  \
        VkResult result = x;                                                           \
        if (result != VK_SUCCESS) {                                                    \
            fprintf(stderr, "[Vulkan Error]: %s\n", vke::vk_result_string(result).c_str()); \
            assert(0);                                                                 \
        }                                                                              \
    }
