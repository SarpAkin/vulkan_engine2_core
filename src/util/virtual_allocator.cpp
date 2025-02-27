#include "virtual_allocator.hpp"
#include "util.hpp"

#include <vk_mem_alloc.h>

namespace vke {

std::optional<VirtualAllocator::Allocation> VirtualAllocator::allocate(u32 size, usize alignment) {
    VmaVirtualAllocationCreateInfo alloc_info = {
        .size = size,
    };

    VmaVirtualAllocation alloc;
    VkDeviceSize offset;
    VkResult res = vmaVirtualAllocate(m_virtual_block, &alloc_info, &alloc, &offset);
    if (res != VK_SUCCESS) return std::nullopt;

    return Allocation{
        .allocation = alloc,
        .offset     = checked_integer_cast<u32>(offset),
        .size       = size,
    };
}

VirtualAllocator::VirtualAllocator(u32 block_size) {
    VmaVirtualBlockCreateInfo info{
        .size = block_size,
    };

    vmaCreateVirtualBlock(&info, &m_virtual_block);
}

VirtualAllocator::~VirtualAllocator() {
    if (m_virtual_block) {
        vmaDestroyVirtualBlock(m_virtual_block);
    }
}

void VirtualAllocator::free(Allocation allocation) {
    vmaVirtualFree(m_virtual_block, allocation.allocation);
}
} // namespace vke