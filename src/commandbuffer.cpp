#include "commandbuffer.hpp"

#include <cassert>
#include <initializer_list>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "fwd.hpp"
#include "pipeline.hpp"
#include "util/util.hpp"
#include "vk_resource.hpp"
#include "vkutil.hpp"
#include "vulkan_context.hpp"

namespace vke {

CommandBuffer::CommandBuffer(bool is_primary, int queue_index) {

    VkCommandPoolCreateInfo p_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_index == -1 ? VulkanContext::get_context()->get_graphics_queue_family() : static_cast<u32>(queue_index),
    };

    VK_CHECK(vkCreateCommandPool(device(), &p_info, nullptr, &m_cmd_pool));

    VkCommandBufferAllocateInfo alloc_info{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_cmd_pool,
        .level              = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        .commandBufferCount = 1,
    };

    VK_CHECK(vkAllocateCommandBuffers(device(), &alloc_info, &m_cmd));
}

CommandBuffer::~CommandBuffer() {
    if (m_is_external) return;

    vkDestroyCommandPool(device(), m_cmd_pool, nullptr);
}

void CommandBuffer::begin() {
    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    VK_CHECK(vkBeginCommandBuffer(m_cmd, &begin_info));
}

void CommandBuffer::end() {
    VK_CHECK(vkEndCommandBuffer(m_cmd));
}

void CommandBuffer::reset() {
    VK_CHECK(vkResetCommandBuffer(m_cmd, 0));
    m_current_pipeline = nullptr;
    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;

    m_wait_semaphores.clear();
    m_dependent_resources.clear();
}

std::span<VkSemaphore> CommandBuffer::get_wait_semaphores() {
    return m_wait_semaphores;
}


// VkCmd** wrappers
void CommandBuffer::cmd_begin_renderpass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
    vkCmdBeginRenderPass(handle(), pRenderPassBegin, contents);
    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void CommandBuffer::cmd_next_subpass(VkSubpassContents contents) {
    vkCmdNextSubpass(handle(), contents);
}

void CommandBuffer::cmd_end_renderpass() {
    vkCmdEndRenderPass(handle());
    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;
}

void CommandBuffer::bind_pipeline(IPipeline* pipeline) {
    if(m_current_pipeline == pipeline) return;

    if(auto rc_ref = pipeline->try_get_reference()) {
        m_dependent_resources.push_back(std::move(rc_ref));
    }

    pipeline->bind(*this);
    m_current_pipeline = pipeline;
}

void CommandBuffer::bind_vertex_buffer(const std::initializer_list<const IBufferSpan*>& buffer) {
    auto handles = MAP_VEC_ALLOCA(buffer, [](const IBufferSpan* buffer) { return buffer->handle(); });
    auto offsets = MAP_VEC_ALLOCA(buffer, [](const IBufferSpan* buffer) { return (VkDeviceSize)buffer->byte_offset(); });

    vkCmdBindVertexBuffers(handle(), 0, buffer.size(), handles.data(), offsets.data());
}

void CommandBuffer::bind_index_buffer(const IBufferSpan* buffer, VkIndexType index_type) {
    vkCmdBindIndexBuffer(handle(), buffer->handle(), buffer->byte_offset(), index_type);
}

void CommandBuffer::bind_descriptor_set(u32 index, VkDescriptorSet set) {
    assert(m_current_pipeline != nullptr && "a pipeline must be bound first before binding a set");

    vkCmdBindDescriptorSets(handle(), m_current_pipeline_state, m_current_pipeline->layout(), index, 1, &set, 0, nullptr);
}

void CommandBuffer::push_constant(u32 size, const void* pValues) {
    assert(m_current_pipeline != nullptr && "a pipeline must be bound first before binding a set");

    vkCmdPushConstants(handle(), m_current_pipeline->layout(), m_current_pipeline->push_stages(), 0, size, pValues);
}

// draw calls
void CommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) {
    vkCmdDraw(handle(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::draw_indexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) {
    vkCmdDrawIndexed(handle(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::draw_indirect(const IBufferSpan* drawcall_buffer, u32 draw_count, u32 stride) {
    vkCmdDrawIndirect(handle(), drawcall_buffer->handle(), drawcall_buffer->byte_offset(), draw_count, stride);
}

void CommandBuffer::draw_indexed_indirect(const IBufferSpan* drawcall_buffer, u32 draw_count, u32 stride) {
    vkCmdDrawIndexedIndirect(handle(), drawcall_buffer->handle(), drawcall_buffer->byte_offset(), draw_count, stride);
}

void CommandBuffer::draw_indirect_count(const IBufferSpan* drawcall_buffer, const IBufferSpan* count_buffer, u32 max_draw_count, u32 stride) {
    vkCmdDrawIndirectCount(handle(), drawcall_buffer->handle(), drawcall_buffer->byte_offset(),
        count_buffer->handle(), count_buffer->byte_offset(), max_draw_count, stride);
}

void CommandBuffer::draw_indexed_indirect_count(const IBufferSpan* drawcall_buffer, const IBufferSpan* count_buffer, u32 max_draw_count, u32 stride) {
    vkCmdDrawIndexedIndirectCount(handle(), drawcall_buffer->handle(), drawcall_buffer->byte_offset(),
        count_buffer->handle(), count_buffer->byte_offset(), max_draw_count, stride);
}

thread_local static PFN_vkCmdDrawMeshTasksEXT _vkCmdDrawMeshTasksEXT = nullptr;
void CommandBuffer::draw_mesh_tasks(u32 group_count_x, u32 group_count_y, u32 group_count_z) {
    if (_vkCmdDrawMeshTasksEXT == nullptr) {
        _vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(device(), "vkCmdDrawMeshTasksEXT");
        assert(_vkCmdDrawMeshTasksEXT && "vkCmdDrawMeshTasksEXT not available");
    }

    _vkCmdDrawMeshTasksEXT(handle(), group_count_x, group_count_y, group_count_z);

    // vkCmdDrawMeshTasksEXT(handle(), group_count_x, group_count_y, group_count_z);
}

thread_local static PFN_vkCmdDrawMeshTasksIndirectEXT _vkCmdDrawMeshTasksIndirectEXT = nullptr;
void CommandBuffer::draw_mesh_tasks_indirect(const IBufferSpan* buffer, u32 draw_count, u32 stride) {
    if(_vkCmdDrawMeshTasksIndirectEXT == nullptr) {
        _vkCmdDrawMeshTasksIndirectEXT = (PFN_vkCmdDrawMeshTasksIndirectEXT)vkGetDeviceProcAddr(device(), "vkCmdDrawMeshTasksIndirectEXT");
        assert(_vkCmdDrawMeshTasksIndirectEXT && "vkCmdDrawMeshTasksIndirectEXT not available");
    }
    
    _vkCmdDrawMeshTasksIndirectEXT(handle(), buffer->handle(), buffer->byte_offset(), draw_count, stride);
}

void CommandBuffer::draw_mesh_tasks_indirect_count(const IBufferSpan* buffer, const IBufferSpan* draw_count_buffer, u32 max_draw_count, u32 stride) {
    // vkCmdDrawMeshTasksIndirectCountEXT(handle(), buffer->handle(), buffer->byte_offset(), draw_count_buffer->handle(), draw_count_buffer->byte_offset(), max_draw_count, stride);
}

void CommandBuffer::dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z) {
    vkCmdDispatch(handle(), group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::pipeline_barrier(const PipelineBarrierArgs& args) {
    vkCmdPipelineBarrier(handle(),
        args.src_stage_mask, args.dst_stage_mask, args.dependency_flags,
        static_cast<uint32_t>(args.memory_barriers.size()), args.memory_barriers.data(),
        static_cast<uint32_t>(args.buffer_memory_barriers.size()), args.buffer_memory_barriers.data(),
        static_cast<uint32_t>(args.image_memory_barriers.size()), args.image_memory_barriers.data());
}

void CommandBuffer::copy_buffer(const vke::IBuffer* src_buffer, const vke::IBuffer* dst_bfufer, std::span<VkBufferCopy> regions) {
    vkCmdCopyBuffer(handle(), src_buffer->handle(), dst_bfufer->handle(), static_cast<u32>(regions.size()), regions.data());
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

// void CommandBuffer::begin_secondry(Renderpass* renderpass, u32 subpass) {
//     VkCommandBufferInheritanceInfo inheritance_info{
//         .sType      = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
//         .renderPass = renderpass->handle(),
//         .subpass    = subpass,
//     };

//     VkCommandBufferBeginInfo info{
//         .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//         .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
//         .pInheritanceInfo = &inheritance_info,
//     };

//     m_current_pipeline_state = VK_PIPELINE_BIND_POINT_GRAPHICS;

//     VK_CHECK(vkBeginCommandBuffer(handle(), &info));

//     renderpass->set_states(*this);
// }

void CommandBuffer::begin_secondry() {
    VkCommandBufferInheritanceInfo inheritance_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
    };

    VkCommandBufferBeginInfo info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = &inheritance_info,
    };

    VK_CHECK(vkBeginCommandBuffer(handle(), &info));
}

void CommandBuffer::execute_secondries(const CommandBuffer* cmd) {
    vkCmdExecuteCommands(handle(), 1, &cmd->m_cmd);
}

void CommandBuffer::execute_secondries(std::span<const CommandBuffer*> cmds) {
    auto handles = MAP_VEC_ALLOCA(cmds, [](const CommandBuffer* cmd) { return cmd->m_cmd; });

    vkCmdExecuteCommands(handle(), handles.size(), handles.data());
}

CommandBuffer::CommandBuffer(VkCommandBuffer cmd,bool is_renderpass, bool is_primary) {
    m_cmd         = cmd;
    m_cmd_pool    = VK_NULL_HANDLE;
    m_is_external = true;

    if(is_renderpass) {
        m_current_pipeline_state = VK_PIPELINE_BIND_POINT_GRAPHICS;
    }else{
        m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;
    }
}

} // namespace vke
