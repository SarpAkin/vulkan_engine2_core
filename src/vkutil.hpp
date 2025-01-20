#pragma once

#ifndef _WIN32
#include <alloca.h>
#else
#include <malloc.h>
#ifndef alloca
#define alloca(x) _alloca(x)
#endif
#endif

#include <cassert>
#include <cstdio>
#include <string>

#include <vulkan/vulkan_core.h>

namespace vke {
std::string vk_result_string(VkResult res);

bool is_depth_format(VkFormat format);

} // namespace vke

#define VK_CHECK(x)                                                                                   \
    {                                                                                                 \
        VkResult __vkchek__result = x;                                                                \
        if (__vkchek__result != VK_SUCCESS) {                                                         \
            fprintf(stderr, "[Vulkan Error]: %s\n", vke::vk_result_string(__vkchek__result).c_str()); \
            assert(0);                                                                                \
        }                                                                                             \
    }
