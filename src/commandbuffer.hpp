#pragma once

#include <initializer_list>
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "vk_resource.hpp"

namespace vke {

class IPipeline;

struct PipelineBarrierArgs {
    VkPipelineStageFlags src_stage_mask, dst_stage_mask;
    // VK_DEPENDENCY_DEVICE_GROUP_BIT by default
    VkDependencyFlags dependency_flags                      = VK_DEPENDENCY_DEVICE_GROUP_BIT;
    std::span<VkMemoryBarrier> memory_barriers              = std::span((VkMemoryBarrier*)nullptr, 0);
    std::span<VkBufferMemoryBarrier> buffer_memory_barriers = std::span((VkBufferMemoryBarrier*)nullptr, 0);
    std::span<VkImageMemoryBarrier> image_memory_barriers   = std::span((VkImageMemoryBarrier*)nullptr, 0);
};

class CommandBuffer : public Resource {
public:
    friend Fence;

    CommandBuffer(bool is_primary = true, int queue_family_index = -1);
    CommandBuffer(VkCommandBuffer cmd, bool is_renderpass = false, bool is_primary = false);

    ~CommandBuffer();

    inline const VkCommandBuffer& handle() { return this->m_cmd; }

    inline void add_execution_dependency(vke::RCResource<Resource> resource) { m_dependent_resources.push_back(std::move(resource)); }

    // vk stuff
    void begin();
    void end();
    void reset();

    std::span<VkSemaphore> get_wait_semaphores();

    void begin_secondary();
    void begin_secondary(const ISubpass* subpass);

    void cmd_begin_renderpass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents);
    void cmd_next_subpass(VkSubpassContents contents);
    void cmd_end_renderpass();

    void execute_secondaries(const CommandBuffer* cmd);
    void execute_secondaries(std::span<const CommandBuffer*> cmd);

    // VkCmd* wrappers
    // void begin_renderpass(Renderpass* renderpass, VkSubpassContents contents);
    // void next_subpass(VkSubpassContents contents) { cmd_next_subpass(contents); };
    // void end_renderpass() { cmd_end_renderpass(); }

    void bind_pipeline(IPipeline* pipeline);
    void bind_index_buffer(const IBufferSpan* buffer, VkIndexType index_type = VK_INDEX_TYPE_UINT16);
    void bind_index_buffer(const IBufferSpan& buffer, VkIndexType index_type = VK_INDEX_TYPE_UINT16) { bind_index_buffer(&buffer, index_type); }

    void bind_vertex_buffer(const std::span<const IBufferSpan*>& buffer);
    void bind_vertex_buffer(const std::initializer_list<const IBufferSpan*>& buffer);
    void bind_descriptor_set(u32 index, VkDescriptorSet set);
    void push_constant(u32 size, const void* pValues);
    template <typename T>
    void push_constant(const T* push) { push_constant(sizeof(T), push); }

    void draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance);
    void draw_indirect(const IBufferSpan* drawcall_buffer, u32 draw_count, u32 stride = sizeof(VkDrawIndirectCommand));
    void draw_indirect_count(const IBufferSpan* drawcall_buffer, const IBufferSpan* count_buffer, u32 max_draw_count, u32 stride = sizeof(VkDrawIndirectCommand));

    void draw_indirect(const IBufferSpan& drawcall_buffer, u32 draw_count, u32 stride = sizeof(VkDrawIndirectCommand)) { draw_indirect(&drawcall_buffer, draw_count, stride); }
    void draw_indirect_count(const IBufferSpan& drawcall_buffer, const IBufferSpan& count_buffer, u32 max_draw_count, u32 stride = sizeof(VkDrawIndirectCommand)) {
        draw_indirect_count(&drawcall_buffer, &count_buffer, max_draw_count, stride);
    }

    void draw_indexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance);
    void draw_indexed_indirect(const IBufferSpan* drawcall_buffer, u32 draw_count, u32 stride = sizeof(VkDrawIndexedIndirectCommand));
    void draw_indexed_indirect_count(const IBufferSpan* drawcall_buffer, const IBufferSpan* count_buffer, u32 max_draw_count, u32 stride = sizeof(VkDrawIndexedIndirectCommand));

    void draw_indexed_indirect(const IBufferSpan& drawcall_buffer, u32 draw_count, u32 stride = sizeof(VkDrawIndexedIndirectCommand)) { draw_indexed_indirect(&drawcall_buffer, draw_count, stride); }
    void draw_indexed_indirect_count(const IBufferSpan& drawcall_buffer, const IBufferSpan& count_buffer, u32 max_draw_count, u32 stride = sizeof(VkDrawIndexedIndirectCommand)) {
        draw_indexed_indirect_count(&drawcall_buffer, &count_buffer, max_draw_count, stride);
    }

    void draw_mesh_tasks(u32 group_count_x, u32 group_count_y, u32 group_count_z);
    void draw_mesh_tasks_indirect(const IBufferSpan* indirect_draw_buffer, u32 draw_count, u32 stride);
    void draw_mesh_tasks_indirect_count(const IBufferSpan* indirect_draw_buffer, const IBufferSpan* draw_count_buffer, u32 max_draw_count, u32 stride);

    void copy_buffer(const vke::IBuffer* src_buffer, const vke::IBuffer* dst_buffer, std::span<VkBufferCopy> regions);
    void copy_buffer(const vke::IBufferSpan& src_span, const vke::IBufferSpan& dst_span);

    void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z);

    void fill_buffer(vke::IBufferSpan& buffer_span, u32 data);

    void pipeline_barrier(const PipelineBarrierArgs& args);

private:
    void flush_postponed_descriptor_sets();

private:
    VkCommandBuffer m_cmd;
    VkCommandPool m_cmd_pool;
    bool m_is_external = false;

    std::vector<RCResource<Resource>> m_dependent_resources;
    std::vector<VkSemaphore> m_wait_semaphores;
    std::vector<std::pair<u32, VkDescriptorSet>> m_postponed_set_binds;

    VkPipelineBindPoint m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;
    IPipeline* m_current_pipeline                = nullptr;
};

} // namespace vke
