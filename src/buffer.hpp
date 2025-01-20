#pragma once

#include <cassert>
#include <vulkan/vulkan_core.h>

#include <span>

#include "fwd.hpp"
#include "vk_resource.hpp"

typedef struct VmaAllocation_T* VmaAllocation;

namespace vke {

class BufferSpan;
class Buffer;
class IBuffer;

class IBufferSpan {
public:
    virtual usize byte_offset() const { return 0; }
    virtual usize byte_size() const { return 0; }
    virtual IBuffer* vke_buffer()             = 0;
    virtual const IBuffer* vke_buffer() const = 0;

    virtual VkBuffer handle() const = 0;

    virtual std::span<u8> mapped_data_bytes() = 0;

    virtual usize bind_size()const{return byte_size();}

    VkDeviceSize device_address() const;

    template <typename T>
    std::span<T> mapped_data() {
        auto bytes = mapped_data_bytes();
        return std::span<T>(reinterpret_cast<T*>(bytes.data()), bytes.size_bytes() / sizeof(T));
    }

    BufferSpan subspan(usize byte_offset, usize byte_size = SIZE_MAX);

    template <typename T>
    BufferSpan subspan_item(usize item_offset, usize item_count);

    virtual ~IBufferSpan() = default;
};

class IBuffer : public IBufferSpan {
public:
    virtual ~IBuffer() = default;
protected:
};

class Buffer : public Resource, public IBuffer {
public:
    Buffer(VkBufferUsageFlags usage, usize buffer_size, bool host_visible /* whether it is accessible by cpu*/);
    ~Buffer();

    VkBuffer handle() const override { return m_buffer; }

    template <typename T = void>
    [[deprecated("use mapped_data")]]
    inline T* mapped_data_ptr() const { return reinterpret_cast<T*>(m_mapped_data); }

    template <typename T>
    // [[deprecated("use mapped_data")]]
    inline std::span<T> mapped_data_as_span() {
        T* data_begin = reinterpret_cast<T*>(m_mapped_data);
        T* data_end   = data_begin + (m_buffer_byte_size / sizeof(T));
        return std::span(data_begin, data_end);
    }

    std::span<u8> mapped_data_bytes() override { return mapped_data_as_span<u8>(); }

public: // overrides
    usize byte_size() const override { return m_buffer_byte_size; }

private:
    IBuffer* vke_buffer() override { return this; }
    const IBuffer* vke_buffer() const override { return this; }

protected:
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    void* m_mapped_data      = nullptr;
    usize m_buffer_byte_size = 0;
};

class BufferSpan : public IBufferSpan {
public:
    BufferSpan(IBuffer* buffer, usize offset, usize byte_size)
        : m_buffer(buffer), m_offset(offset), m_byte_size(byte_size) {
        assert(m_offset + m_byte_size <= m_buffer->byte_size());
    }

    usize byte_offset() const override { return m_offset; }
    usize byte_size() const override { return m_byte_size; }
    IBuffer* vke_buffer() override { return m_buffer; };
    const IBuffer* vke_buffer() const override { return m_buffer; };

    VkBuffer handle() const override { return m_buffer->handle(); }

    std::span<u8> mapped_data_bytes() override { return vke_buffer()->mapped_data_bytes().subspan(m_offset, m_byte_size); }

private:
    IBuffer* m_buffer;
    usize m_offset, m_byte_size;
};

template <typename T>
BufferSpan IBufferSpan::subspan_item(usize item_offset, usize item_count) {
    return subspan(item_offset * sizeof(T), item_count * sizeof(T));
}

} // namespace vke