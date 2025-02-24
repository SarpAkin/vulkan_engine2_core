#pragma once

#include <unordered_map>
#include <vector>

#include <vke/fwd.hpp>

#include "../buffer.hpp"
#include "slim_vec.hpp"

#include "hash_util.hpp"
#include "util.hpp"

namespace vke {

class StencilBuffer {
public:
    // 256KiB block size by default. not enough for images
    StencilBuffer(u32 block_size = 1 << 18, bool growable = true);

    BufferSpan allocate(u32 byte_size, bool allow_grow = true);

    // requires flush_copies to be called to have data actually be copied
    void copy_data(BufferSpan destination, std::span<const u8> data);

    template <class T>
    void copy_data(BufferSpan destination, const T* data, usize count /*not in bytes rather in amount*/) {
        copy_data(destination, std::span<const u8>(reinterpret_cast<const u8*>(data), count * sizeof(T)));
    }

    template <class T>
    void copy_data(BufferSpan destination, std::span<const T> data) {
        copy_data(destination, vke::span_cast<const u8>(data));
    }

    void flush_copies(vke::CommandBuffer& cmd);

private:
    vke::Buffer* get_top_buffer();
    void push_new_buffer();

private:
    // std::unordered_map<Buffer*, vke::SlimVec<VkBufferCopy>> m_copies;

    // pair as dst buffer,src buffer
    std::unordered_map<std::pair<IBuffer*, IBuffer*>, vke::SlimVec<VkBufferCopy>> m_copies;
    std::vector<std::unique_ptr<vke::Buffer>> m_buffers;
    u32 m_top             = 0;
    u32 m_buffer_capacity = 0;
    bool m_growable;
};

} // namespace vke