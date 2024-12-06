#include "frame_buffer.hpp"

namespace vke {
namespace impl {

Framebuffer::~Framebuffer() {
    vkDestroyFramebuffer(device(), m_handle, nullptr);
}
} // namespace impl
} // namespace vke