#include "vk_resource.hpp"

#include "vulkan_context.hpp"
#include <cassert>

namespace vke {

VkDevice DeviceGetter::device() const {
    return VulkanContext::get_context()->get_device();
}

VulkanContext* DeviceGetter::get_context() { return VulkanContext::get_context(); }


Resource::Resource() {}
Resource::~Resource() {}

RCResource<Resource> Resource::get_reference() {
    if(!m_is_reference_counted){
        assert(!"resource must be reference counted");
        abort();
    }
    
    increment_reference_count();

    return RCResource(this);
}
} // namespace vke