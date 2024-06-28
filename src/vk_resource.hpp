#pragma once

#include "fwd.hpp"
#include "common.hpp" // IWYU pragma: export

#include <vulkan/vulkan_core.h>

namespace vke {

class Resource {
public:

    Resource();
    virtual ~Resource();

    Resource(const Resource&)            = delete;
    Resource(Resource&&)                 = delete;
    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&&)      = delete;

protected:
    VkDevice device() const;
    static VulkanContext* get_context();
};

} // namespace vke