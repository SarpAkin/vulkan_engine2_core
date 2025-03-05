#pragma once

#include "vke/fwd.hpp"
#include <optional>

typedef struct VmaVirtualAllocation_T* VmaVirtualAllocation;
typedef struct VmaVirtualBlock_T* VmaVirtualBlock;

namespace vke {

// a wrapper around VmaVirtualAllocator
class VirtualAllocator {
public:
    struct Allocation {
        VmaVirtualAllocation allocation = nullptr;
        u32 offset                      = 0;
        u32 size                        = 0;
    };

public:
    std::optional<Allocation> allocate(u32 size, usize alignment = 1);
    void free(Allocation allocation);
    void reset();

    VmaVirtualBlock get_virtual_block() { return m_virtual_block; }

public:
    VirtualAllocator(u32 block_size);
    ~VirtualAllocator();

    VirtualAllocator(const VirtualAllocator& other)            = delete;
    VirtualAllocator& operator=(const VirtualAllocator& other) = delete;

    VirtualAllocator(VirtualAllocator&& other) {
        m_virtual_block       = other.m_virtual_block;
        other.m_virtual_block = nullptr;
    }

    VirtualAllocator& operator=(VirtualAllocator&& other) {
        m_virtual_block       = other.m_virtual_block;
        other.m_virtual_block = nullptr;
        return *this;
    }

private:
    VmaVirtualBlock m_virtual_block = nullptr;
};

}; // namespace vke