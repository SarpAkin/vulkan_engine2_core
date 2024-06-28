#include "buffer.hpp"

#include "vkutil.hpp"
#include "vulkan_context.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace vke {

Buffer::Buffer(VkBufferUsageFlags usage, usize buffer_size, bool host_visible) {
    m_buffer_byte_size = buffer_size;

    VkBufferCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = buffer_size,
        .usage = usage,
    };

    VmaAllocationCreateInfo alloc_info{
        .flags = host_visible ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0u,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VK_CHECK(vmaCreateBuffer(get_context()->gpu_allocator(), &create_info, &alloc_info, &m_buffer, &m_allocation, nullptr));
    vmaSetAllocationName(get_context()->gpu_allocator(), m_allocation, "image");

    if (host_visible) {
        VK_CHECK(vmaMapMemory(get_context()->gpu_allocator(), m_allocation, &m_mapped_data));
    }
}

Buffer::~Buffer() {

    if (m_mapped_data) {
        vmaUnmapMemory(get_context()->gpu_allocator(), m_allocation);
    }

    vmaDestroyBuffer(get_context()->gpu_allocator(), m_buffer, m_allocation);
}

BufferSpan IBufferSpan::subspan(usize _byte_offset, usize _byte_size) {
    return BufferSpan(vke_buffer(), byte_offset() + _byte_offset, std::min(_byte_size, byte_size() - _byte_offset));
}

} // namespace vke