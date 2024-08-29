#include "arena_alloc.hpp"


#ifndef _WIN32
#include <sys/mman.h>
#else
#include <Windows.h>
#endif

// 4GB
const usize HEAP_SIZE = 1l << 32;

namespace vke {

ArenaAllocator::ArenaAllocator() {
#ifndef _WIN32
    m_base = reinterpret_cast<u8*>(mmap(nullptr, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
#else
    m_base = reinterpret_cast<u8*>(VirtualAlloc(nullptr, HEAP_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

#endif
    
    m_top  = m_base;
    m_cap  = m_base + HEAP_SIZE;
}

ArenaAllocator::~ArenaAllocator() {
#ifndef _WIN32
    munmap(m_base, HEAP_SIZE);
#else
    VirtualFree(m_base, 0, MEM_RELEASE);
#endif
}


constexpr usize align_up(usize value, usize alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void* ArenaAllocator::alloc(usize size) {
    // Check if there is enough space in the arena to accommodate the requested size
    if (m_top + size > m_cap) {
        // If not, you can handle the out-of-memory situation according to your requirements.
        // For example, you can throw an exception or return nullptr.
        // In this example, let's return nullptr.
        return nullptr;
    }

    size = align_up(size, 8);

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
