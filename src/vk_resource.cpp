#include "vk_resource.hpp"

#include "vulkan_context.hpp"

namespace vke {

VkDevice DeviceGetter::device() const {
    return VulkanContext::get_context()->get_device();
}

VulkanContext* DeviceGetter::get_context() { return VulkanContext::get_context(); }


Resource::Resource() {}
Resource::~Resource() {}
} // namespace vke