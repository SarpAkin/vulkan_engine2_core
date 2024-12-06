#pragma once

#include <vulkan/vulkan_core.h>

#include <vke/fwd.hpp>

#include "../vk_resource.hpp"

namespace vke {
namespace impl {

class Framebuffer : public vke::Resource {
public:
    VkFramebuffer handle() { return m_handle; }
    Framebuffer(VkFramebuffer fb) : m_handle(fb) {}
    ~Framebuffer();

private:
    VkFramebuffer m_handle;
};

} // namespace impl
} // namespace vke