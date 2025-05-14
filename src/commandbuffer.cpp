#include "commandbuffer.hpp"

#include <cassert>
#include <initializer_list>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "fwd.hpp"
#include "isubpass.hpp"
#include "pipeline.hpp"
#include "renderpass/renderpass.hpp"
#include "util/function_timer.hpp"
#include "util/util.hpp"
#include "vk_resource.hpp"
#include "vkutil.hpp"
#include "vulkan_context.hpp"
#include "command_pool.hpp"

namespace vke {

CommandBuffer::CommandBuffer(bool is_primary, int queue_index) {
    m_dt = &get_dispatch_table();

    VkCommandPoolCreateInfo p_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_index == -1 ? VulkanContext::get_context()->get_graphics_queue_family() : static_cast<u32>(queue_index),
    };

    VK_CHECK(m_dt->vkCreateCommandPool(device(), &p_info, nullptr, &m_cmd_pool));

    VkCommandBufferAllocateInfo alloc_info{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_cmd_pool,
        .level              = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        .commandBufferCount = 1,
    };

    VK_CHECK(m_dt->vkAllocateCommandBuffers(device(), &alloc_info, &m_cmd));

    m_is_primary = is_primary;
}

CommandBuffer::CommandBuffer(CommandPool* pool, VkCommandBuffer cmd,bool is_primary) {
    m_dt = &get_dispatch_table();

    m_vke_cmd_pool = pool;
    m_cmd = cmd;
    m_is_primary = is_primary;
}

CommandBuffer::~CommandBuffer() {
    if (m_is_external) return;

    if(m_vke_cmd_pool){
        m_dt->vkResetCommandBuffer(m_cmd,0);
        m_vke_cmd_pool->push_recycled_cmd(m_cmd, m_is_primary);
    }else{
        m_dt->vkDestroyCommandPool(device(), m_cmd_pool, nullptr);
    }
}

void CommandBuffer::begin() {
    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    VK_CHECK(m_dt->vkBeginCommandBuffer(m_cmd, &begin_info));
}

void CommandBuffer::end() {
    VK_CHECK(vkEndCommandBuffer(m_cmd));
}

void CommandBuffer::reset() {
    VK_CHECK(vkResetCommandBuffer(m_cmd, 0));
    m_current_pipeline       = nullptr;
    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;

    m_wait_semaphores.clear();
    m_dependent_resources.clear();
}

std::span<VkSemaphore> CommandBuffer::get_wait_semaphores() {
    return m_wait_semaphores;
}

// m_dispatch_table->vkCmd** wrappers
void CommandBuffer::cmd_begin_renderpass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
    m_dt->vkCmdBeginRenderPass(handle(), pRenderPassBegin, contents);
    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_GRAPHICS;

    m_postponed_set_binds.clear();
}

void CommandBuffer::cmd_next_subpass(VkSubpassContents contents) {
    m_dt->vkCmdNextSubpass(handle(), contents);

    m_postponed_set_binds.clear();
}

void CommandBuffer::cmd_end_renderpass() {
    m_dt->vkCmdEndRenderPass(handle());
    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;

    m_postponed_set_binds.clear();
}

void CommandBuffer::bind_pipeline(IPipeline* pipeline) {
    if (m_current_pipeline == pipeline) return;

    pipeline->bind(*this);
    m_current_pipeline = pipeline;

    if (auto rc_ref = pipeline->try_get_reference()) {
        m_dependent_resources.push_back(std::move(rc_ref));
    }

    flush_postponed_descriptor_sets();
}

void CommandBuffer::bind_vertex_buffer(std::span<const VkBuffer> buffers, std::span<const VkDeviceSize> offsets) {
    m_dt->vkCmdBindVertexBuffers(handle(), 0, buffers.size(), buffers.data(), offsets.data());
}

void CommandBuffer::bind_vertex_buffer(std::span<const std::unique_ptr<IBufferSpan>> buffer) {
    auto handles = MAP_VEC_ALLOCA(buffer, [](const std::unique_ptr<IBufferSpan>& buffer) { return buffer->handle(); });
    auto offsets = MAP_VEC_ALLOCA(buffer, [](const std::unique_ptr<IBufferSpan>& buffer) { return (VkDeviceSize)buffer->byte_offset(); });

    m_dt->vkCmdBindVertexBuffers(handle(), 0, buffer.size(), handles.data(), offsets.data());
}


void CommandBuffer::bind_vertex_buffer(const std::span<const IBufferSpan*>& buffer) {
    auto handles = MAP_VEC_ALLOCA(buffer, [](const IBufferSpan* buffer) { return buffer->handle(); });
    auto offsets = MAP_VEC_ALLOCA(buffer, [](const IBufferSpan* buffer) { return (VkDeviceSize)buffer->byte_offset(); });

    m_dt->vkCmdBindVertexBuffers(handle(), 0, buffer.size(), handles.data(), offsets.data());
}

void CommandBuffer::bind_vertex_buffer(const std::initializer_list<const IBufferSpan*>& buffer) {
    auto handles = MAP_VEC_ALLOCA(buffer, [](const IBufferSpan* buffer) { return buffer->handle(); });
    auto offsets = MAP_VEC_ALLOCA(buffer, [](const IBufferSpan* buffer) { return (VkDeviceSize)buffer->byte_offset(); });

    m_dt->vkCmdBindVertexBuffers(handle(), 0, buffer.size(), handles.data(), offsets.data());
}

void CommandBuffer::bind_index_buffer(const IBufferSpan* buffer, VkIndexType index_type) {
    m_dt->vkCmdBindIndexBuffer(handle(), buffer->handle(), buffer->byte_offset(), index_type);
}

void CommandBuffer::bind_descriptor_set(u32 index, VkDescriptorSet set) {
    // assert(m_current_pipeline != nullptr && "a pipeline must be bound first before binding a set");
    if (m_current_pipeline != nullptr) {
        m_dt->vkCmdBindDescriptorSets(handle(), m_current_pipeline_state, m_current_pipeline->layout(), index, 1, &set, 0, nullptr);
    } else {
        m_postponed_set_binds.push_back(std::pair(index, set));
    }
}

void CommandBuffer::push_constant(u32 size, const void* pValues) {
    assert(m_current_pipeline != nullptr && "a pipeline must be bound first before binding a set");

    m_dt->vkCmdPushConstants(handle(), m_current_pipeline->layout(), m_current_pipeline->push_stages(), 0, size, pValues);
}

// draw calls
void CommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) {
    m_dt->vkCmdDraw(handle(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::draw_indexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) {
    m_dt->vkCmdDrawIndexed(handle(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::draw_indirect(const IBufferSpan* drawcall_buffer, u32 draw_count, u32 stride) {
    m_dt->vkCmdDrawIndirect(handle(), drawcall_buffer->handle(), drawcall_buffer->byte_offset(), draw_count, stride);
}

void CommandBuffer::draw_indexed_indirect(const IBufferSpan* drawcall_buffer, u32 draw_count, u32 stride) {
    m_dt->vkCmdDrawIndexedIndirect(handle(), drawcall_buffer->handle(), drawcall_buffer->byte_offset(), draw_count, stride);
}

void CommandBuffer::draw_indirect_count(const IBufferSpan* drawcall_buffer, const IBufferSpan* count_buffer, u32 max_draw_count, u32 stride) {
    m_dt->vkCmdDrawIndirectCount(handle(), drawcall_buffer->handle(), drawcall_buffer->byte_offset(),
        count_buffer->handle(), count_buffer->byte_offset(), max_draw_count, stride);
}

void CommandBuffer::draw_indexed_indirect_count(const IBufferSpan* drawcall_buffer, const IBufferSpan* count_buffer, u32 max_draw_count, u32 stride) {
    m_dt->vkCmdDrawIndexedIndirectCount(handle(), drawcall_buffer->handle(), drawcall_buffer->byte_offset(),
        count_buffer->handle(), count_buffer->byte_offset(), max_draw_count, stride);
}

void CommandBuffer::draw_mesh_tasks(u32 group_count_x, u32 group_count_y, u32 group_count_z) {
    m_dt->vkCmdDrawMeshTasksEXT(handle(), group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::draw_mesh_tasks_indirect(const IBufferSpan* buffer, u32 draw_count, u32 stride) {
    m_dt->vkCmdDrawMeshTasksIndirectEXT(handle(), buffer->handle(), buffer->byte_offset(), draw_count, stride);
}

void CommandBuffer::draw_mesh_tasks_indirect_count(const IBufferSpan* buffer, const IBufferSpan* draw_count_buffer, u32 max_draw_count, u32 stride) {
    m_dt->vkCmdDrawMeshTasksIndirectCountEXT(handle(), buffer->handle(), buffer->byte_offset(), draw_count_buffer->handle(), draw_count_buffer->byte_offset(), max_draw_count, stride);
}

void CommandBuffer::dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z) {
    m_dt->vkCmdDispatch(handle(), group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::pipeline_barrier(const PipelineBarrierArgs& args) {
    m_dt->vkCmdPipelineBarrier(handle(),
        args.src_stage_mask, args.dst_stage_mask, args.dependency_flags,
        static_cast<uint32_t>(args.memory_barriers.size()), args.memory_barriers.data(),
        static_cast<uint32_t>(args.buffer_memory_barriers.size()), args.buffer_memory_barriers.data(),
        static_cast<uint32_t>(args.image_memory_barriers.size()), args.image_memory_barriers.data());
}

void CommandBuffer::copy_buffer(const vke::IBuffer* src_buffer, const vke::IBuffer* dst_buffer, std::span<VkBufferCopy> regions) {
    m_dt->vkCmdCopyBuffer(handle(), src_buffer->handle(), dst_buffer->handle(), static_cast<u32>(regions.size()), regions.data());
}
void CommandBuffer::copy_buffer(const vke::IBufferSpan& src_span, const vke::IBufferSpan& dst_span) {
    VkBufferCopy copies[] = {
        VkBufferCopy{
            .srcOffset = src_span.byte_offset(),
            .dstOffset = dst_span.byte_offset(),
            .size      = src_span.byte_size(),
        },
    };

    copy_buffer(src_span.vke_buffer(), dst_span.vke_buffer(), copies);
}

void CommandBuffer::begin_secondary(const ISubpass* subpass) {
    auto* renderpass = subpass->get_vke_renderpass();

    VkCommandBufferInheritanceInfo inheritance_info{
        .sType      = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .renderPass = subpass->get_renderpass_handle(),
        .subpass    = subpass->get_subpass_index(),
    };

    VkCommandBufferBeginInfo info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritance_info,
    };

    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VK_CHECK(m_dt->vkBeginCommandBuffer(handle(), &info));

    renderpass->set_states(*this);
}

void CommandBuffer::begin_secondary() {
    VkCommandBufferInheritanceInfo inheritance_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
    };

    VkCommandBufferBeginInfo info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = &inheritance_info,
    };

    VK_CHECK(m_dt->vkBeginCommandBuffer(handle(), &info));
}

void CommandBuffer::execute_secondaries(const CommandBuffer* cmd) {
    m_dt->vkCmdExecuteCommands(handle(), 1, &cmd->m_cmd);
}

void CommandBuffer::execute_secondaries(std::span<const CommandBuffer*> cmds) {
    auto handles = MAP_VEC_ALLOCA(cmds, [](const CommandBuffer* cmd) { return cmd->m_cmd; });

    m_dt->vkCmdExecuteCommands(handle(), handles.size(), handles.data());
}

CommandBuffer::CommandBuffer(VkCommandBuffer cmd, bool is_renderpass, bool is_primary) {
    m_cmd         = cmd;
    m_cmd_pool    = VK_NULL_HANDLE;
    m_is_external = true;

    if (is_renderpass) {
        m_current_pipeline_state = VK_PIPELINE_BIND_POINT_GRAPHICS;
    } else {
        m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;
    }
}

void CommandBuffer::flush_postponed_descriptor_sets() {
    if (m_postponed_set_binds.empty()) return;

    auto layout = m_current_pipeline->layout();
    for (auto& [index, set] : m_postponed_set_binds) {
        m_dt->vkCmdBindDescriptorSets(handle(), m_current_pipeline_state, layout, index, 1, &set, 0, nullptr);
    }

    m_postponed_set_binds.clear();
}
void CommandBuffer::fill_buffer(vke::IBufferSpan& buffer_span, u32 data) {
    m_dt->vkCmdFillBuffer(handle(), buffer_span.handle(), buffer_span.byte_offset(), buffer_span.byte_size(), data);
}
} // namespace vke
