#include "stencil_buffer.hpp"

#include "../commandbuffer.hpp"
#include "util.hpp"

namespace vke {

StencilBuffer::StencilBuffer(u32 block_size, bool growable) {
    m_growable        = growable;
    m_buffer_capacity = block_size;
}

BufferSpan StencilBuffer::allocate(u32 byte_size, bool allow_grow) {
    if (m_top + byte_size > m_buffer_capacity) {
        if (!allow_grow) {
            THROW_ERROR("failed to allocate %d bytes. available space %d", byte_size, m_buffer_capacity - m_top);
        }

        push_new_buffer();
        return allocate(byte_size, false);
    }

    auto bspan = get_top_buffer()->subspan(m_top, byte_size);
    m_top += byte_size;
    return bspan;
}

void StencilBuffer::push_new_buffer() {
    if (m_buffers.size() > 0 && !m_growable) {
        THROW_ERROR("failed to grow buffer. buffer isn't growable!");
    }

    m_buffers.push_back(std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_buffer_capacity, true));
    m_top = 0;
}

vke::Buffer* StencilBuffer::get_top_buffer() {
    if (m_buffers.empty()) push_new_buffer();
    return m_buffers.back().get();
}

void StencilBuffer::copy_data(BufferSpan destination, std::span<const u8> data) {
    assert(destination.byte_size() >= data.size_bytes());

    auto allocation = allocate(data.size());
    memcpy(allocation.mapped_data_bytes().data(), data.data(), data.size_bytes());

    m_copies[std::make_pair(destination.vke_buffer(), allocation.vke_buffer())].push_back(VkBufferCopy{
        .srcOffset = allocation.byte_offset(),
        .dstOffset = destination.byte_offset(),
        .size      = data.size_bytes(),
    });
}

void StencilBuffer::flush_copies(vke::CommandBuffer& cmd) {
    for (auto& [buffer_pair, copies] : m_copies) {
        auto [dst_buffer, src_buffer] = buffer_pair;
        cmd.copy_buffer(src_buffer, dst_buffer, copies);
    }

    for(auto buffer : m_buffers){
        cmd.add_execution_dependency(buffer->get_reference());
    }    
}
} // namespace vke