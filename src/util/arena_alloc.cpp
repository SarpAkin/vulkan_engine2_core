#include "arena_alloc.hpp"

#include <sys/mman.h>

// 4GB
const usize HEAP_SIZE = 1l << 32;

namespace vke {

ArenaAllocator::ArenaAllocator() {
    m_base = reinterpret_cast<u8*>(mmap(nullptr, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    m_top  = m_base;
    m_cap  = m_base + HEAP_SIZE;
}

ArenaAllocator::~ArenaAllocator() {
    munmap(m_base, HEAP_SIZE);
}

void* ArenaAllocator::alloc(usize size) {
    // Check if there is enough space in the arena to accommodate the requested size
    if (m_top + size > m_cap) {
        // If not, you can handle the out-of-memory situation according to your requirements.
        // For example, you can throw an exception or return nullptr.
        // In this example, let's return nullptr.
        return nullptr;
    }

    // Allocate memory from the arena by moving the top pointer
    void* allocated_memory = m_top;
    m_top += size;

    return allocated_memory;
}

const char* ArenaAllocator::create_str_copy(const char* str, usize* out_len) {
    size_t len = strlen(str);

    if (out_len) {
        *out_len = len;
    }

    char* copy = alloc<char>(len);
    memcpy(copy, str, len);
    return copy;
}
} // namespace vke

// m_base = reinterpret_cast<u8*>(mmap(nullptr, HEAP_SIZE, 0, MAP_ANON, 0, 0));
