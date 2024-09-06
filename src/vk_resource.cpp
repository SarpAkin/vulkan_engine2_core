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
    auto ref = try_get_reference();

    assert(ref.get() != nullptr);

    return RCResource(this);
}

RCResource<Resource> Resource::try_get_reference() {
    switch (m_ownership) {
    case OwnerShip::OWNED:
        return RCResource<Resource>();
    case OwnerShip::RefCounted:
        increment_reference_count();
        return RCResource(this);
    case OwnerShip::EXTERNAL:
        return m_external->try_get_reference();
    }

    assert(false);
    return RCResource<Resource>();
}
} // namespace vke