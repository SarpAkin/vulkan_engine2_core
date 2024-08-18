#pragma once

#include "common.hpp" // IWYU pragma: export
#include "fwd.hpp"

#include <vulkan/vulkan_core.h>

namespace vke {

class DeviceGetter {
protected:
    VkDevice device() const;
    static VulkanContext* get_context();

    DeviceGetter()=default;
    ~DeviceGetter()=default;
};

class Resource : public DeviceGetter {
public:
    Resource();
    virtual ~Resource();

    Resource(const Resource&)            = delete;
    Resource(Resource&&)                 = delete;
    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&&)      = delete;

protected:
};



} // namespace vke