#include "vk_resource.hpp"

#include "vulkan_context.hpp"

namespace vke {

VkDevice Resource::device() const {
    return VulkanContext::get_context()->get_device();
}

Resource::Resource() {}
Resource::~Resource() {}
VulkanContext* Resource::get_context() { return VulkanContext::get_context(); }
} // namespace vke