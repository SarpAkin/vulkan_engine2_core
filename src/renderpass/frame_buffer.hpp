#pragma once

#include <vulkan/vulkan_core.h>

#include <vke/fwd.hpp>

#include "../vk_resource.hpp"
#include "../util/slim_vec.hpp"

namespace vke {
namespace impl {

class Framebuffer : public vke::Resource {
public:
    VkFramebuffer handle() { return m_handle; }
    Framebuffer(VkFramebuffer fb) : m_handle(fb) {}
    ~Framebuffer();

    void add_execution_dependency(vke::RCResource<Resource> resource) { m_dependent_resources.push_back(std::move(resource)); }
private:
    VkFramebuffer m_handle;
    //mainly exists for images
    vke::SlimVec<RCResource<Resource>> m_dependent_resources;
};

} // namespace impl
} // namespace vke