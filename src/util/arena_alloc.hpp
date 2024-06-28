#pragma once

#include "../common.hpp"

#include <cstring>
#include <span>

namespace vke {
class ArenaAllocator {
public:
    ArenaAllocator();
    ~ArenaAllocator();

    void* alloc(usize size);

    template <typename T>
    T* alloc(usize count = 1) { return reinterpret_cast<T*>(alloc(count * sizeof(T))); }

    template <typename T>
    std::span<T> create_copy(std::span<const T> src) {
        T* dst = alloc<T>(src.size());
        memcpy(dst, src.data(), src.size_bytes());
        return std::span<T>(dst, src.size());
    }

    template <typename T>
    std::span<T> create_copy(std::span<T> src) {
        T* dst = alloc<T>(src.size());
        memcpy(dst, src.data(), src.size_bytes());
        return std::span<T>(dst, src.size());
    }

    template <typename T>
    T* create_copy(const T& src) {
        T* dst = alloc<T>(1);
        memcpy(dst, &src, sizeof(T));
        return dst;
    }

    const char* create_str_copy(const char* str, usize* out_len = nullptr);

    ArenaAllocator(const ArenaAllocator&)            = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

private:
    u8* m_base;
    u8* m_top;
    u8* m_cap;
};

class ArenaGen {
};

} // namespace vke