#include "command_pool.hpp"

#include <vulkan/vulkan.hpp>

#include "commandbuffer.hpp"
#include "util/util.hpp"
#include "vkutil.hpp"
#include "vulkan_context.hpp"

namespace vke {

CommandPool::CommandPool(int queue_index) {
    VkCommandPoolCreateInfo p_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_index == -1 ? VulkanContext::get_context()->get_graphics_queue_family() : static_cast<u32>(queue_index),
    };

    VK_CHECK(dt().vkCreateCommandPool(device(), &p_info, nullptr, &m_command_pool));
}

CommandPool::~CommandPool() {
    dt().vkDestroyCommandPool(device(), m_command_pool, nullptr);
}

VkCommandBuffer CommandPool::_allocate(bool is_primary) {
    if (auto val = try_pop_front(is_primary ? m_recycled_primary_buffers : m_recycled_secondary_buffers)) {
        return val.value();
    }

    VkCommandBufferAllocateInfo alloc_info{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_command_pool,
        .level              = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd;
    VK_CHECK(dt().vkAllocateCommandBuffers(device(), &alloc_info, &cmd));

    return cmd;
}

std::unique_ptr<CommandBuffer> CommandPool::allocate(bool is_primary) {
    return std::make_unique<CommandBuffer>(this, _allocate(is_primary),is_primary);
}

} // namespace vke