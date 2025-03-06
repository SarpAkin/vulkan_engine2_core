#include "virtual_allocator.hpp"
#include "util.hpp"

#include <vk_mem_alloc.h>

namespace vke {

std::optional<VirtualAllocator::Allocation> VirtualAllocator::allocate(u32 size, usize alignment) {
    VmaVirtualAllocationCreateInfo alloc_info = {
        .size = size,
    };

    VmaVirtualAllocation alloc;
    VkDeviceSize _offset;
    VkResult res = vmaVirtualAllocate(m_virtual_block, &alloc_info, &alloc, &_offset);
    if (res != VK_SUCCESS) {
        return std::nullopt;
    }

    u32 offset = checked_integer_cast<u32>(_offset);

    m_max = std::max(m_max,offset + size);

    return Allocation{
        .allocation = alloc,
        .offset     = offset,
        .size       = size,
    };
}

void VirtualAllocator::reset() {
    vmaClearVirtualBlock(m_virtual_block);
}

VirtualAllocator::VirtualAllocator(u32 block_size) {
    VmaVirtualBlockCreateInfo info{
        .size = block_size,
    };

    m_capacity = block_size;

    vmaCreateVirtualBlock(&info, &m_virtual_block);
}

VirtualAllocator::~VirtualAllocator() {
    if (m_virtual_block) {
        if (!vmaIsVirtualBlockEmpty(m_virtual_block)) {
            LOG_WARNING("not all virtual allocations has been freed before the destruction of virtual allocator. either free each of them or call reset on virtual allocator");
            vmaClearVirtualBlock(m_virtual_block);
        }

        vmaDestroyVirtualBlock(m_virtual_block);
    }
}

void VirtualAllocator::free(Allocation allocation) {
    vmaVirtualFree(m_virtual_block, allocation.allocation);
}

} // namespace vke