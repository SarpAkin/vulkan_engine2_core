#include "growable_buffer.hpp"

#include <vk_mem_alloc.h>

#include "../vulkan_context.hpp"
#include <vke/util.hpp>

namespace vke {

GrowableBuffer::GrowableBuffer(VkBufferUsageFlags usage, usize buffer_size, bool host_visible, usize block_size) {
    auto* ctx = VulkanContext::get_context();

    VkBufferCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT | VK_BUFFER_CREATE_SPARSE_BINDING_BIT,
        .size  = ctx->get_device_info()->properties.limits.maxStorageBufferRange,
        .usage = usage,
    };

    vkCreateBuffer(ctx->get_device(), &create_info, nullptr, &m_buffer);

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(ctx->get_device(), m_buffer, &memory_requirements);

    if (block_size == 0) {
        block_size = memory_requirements.alignment;
        LOG_INFO("no block size specified. using %lu", block_size);
    } else {
        block_size = round_up_to_multiple(block_size, memory_requirements.alignment);
    }
    m_block_size = block_size;

    resize(buffer_size);
}

void GrowableBuffer::resize(usize new_size) {
    if (new_size <= m_buffer_size) {
        LOG_WARNING("new size %lu is smaller than current size %lu. doing nothing.", new_size, m_buffer_size);
        return;
    }

    auto* ctx       = VulkanContext::get_context();
    auto* gpu_alloc = ctx->gpu_allocator();

    new_size = round_up_to_multiple(new_size, m_block_size);

    usize block_count = (new_size - m_buffer_size) / m_block_size;

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(ctx->get_device(), m_buffer, &memory_requirements);

    memory_requirements.size = m_block_size;

    std::vector<VkSparseMemoryBind> m_sparse_infos;
    m_sparse_infos.reserve(block_count);

    usize buffer_cursor = m_buffer_size;

    for (usize i = 0; i < block_count; ++i) {
        VmaAllocationCreateInfo alloc_cinfo = {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        };

        VmaAllocation allocation;
        VmaAllocationInfo alloc_info;

        vmaAllocateMemory(gpu_alloc, &memory_requirements, &alloc_cinfo, &allocation, &alloc_info);

        if (alloc_info.size != m_block_size) {
            assert(alloc_info.size > m_block_size);

            LOG_INFO("allocated extra %ld bytes for block %lu", alloc_info.size - m_block_size, i);
        }

        usize bind_size = m_block_size;

        m_sparse_infos.push_back(VkSparseMemoryBind{
            .resourceOffset = buffer_cursor,
            .size           = bind_size,
            .memory         = alloc_info.deviceMemory,
            .memoryOffset   = alloc_info.offset,
        });

        buffer_cursor += bind_size;

        if (bind_size > new_size) break;
    }

    m_buffer_size = buffer_cursor;

    VkSparseBufferMemoryBindInfo buffer_bind_info = {
        .buffer    = m_buffer,
        .bindCount = uint32_t(m_sparse_infos.size()),
        .pBinds    = m_sparse_infos.data(),
    };

    VkBindSparseInfo sparse_bind_info = {
        .sType           = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,
        .bufferBindCount = 1,
        .pBufferBinds    = &buffer_bind_info,
    };

    auto fence = ctx->get_thread_local_fence();
    VK_CHECK(vkQueueBindSparse(ctx->get_graphics_queue(), 1, &sparse_bind_info, fence));
    auto start = std::chrono::high_resolution_clock::now();

    VK_CHECK(vkWaitForFences(ctx->get_device(), 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
    VK_CHECK(vkResetFences(ctx->get_device(), 1, &fence));

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    LOG_INFO("waited %fµs for sparse buffer bind", static_cast<double>(duration) / 1E3);
}
} // namespace vke