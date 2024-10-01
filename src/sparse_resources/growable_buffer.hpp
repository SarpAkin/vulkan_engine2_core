#pragma once

#include "../buffer.hpp"

#include <vector>

namespace vke {

class GrowableBuffer final : public IBuffer, public Resource {
public:
    GrowableBuffer(VkBufferUsageFlags usage, usize buffer_size, bool host_visible = false, usize block_size = 0);
    ~GrowableBuffer() {}

    usize byte_offset() const override { return 0; }
    usize byte_size() const override { return m_buffer_size; }
    IBuffer* vke_buffer() override { return this; };
    const IBuffer* vke_buffer() const override { return this; }
    VkBuffer handle() const override { return m_buffer; }
    std::span<u8> mapped_data_bytes() override { assert(!"mapping not supported"); }

public:
    void resize(usize new_size);

private:
    VkBuffer m_buffer   = VK_NULL_HANDLE;
    usize m_buffer_size = 0, m_block_size = 0;

    std::vector<VmaAllocation> m_allocations;
};

} // namespace vke