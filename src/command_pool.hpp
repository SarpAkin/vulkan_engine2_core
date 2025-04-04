#pragma once

#include <deque>
#include <memory>
#include <vulkan/vulkan.h>

#include "fwd.hpp"
#include "vk_resource.hpp"

namespace vke {

class CommandPool : Resource {
    friend CommandBuffer;

public:
    CommandPool(int queue_family_index = -1);
    ~CommandPool();

    std::unique_ptr<CommandBuffer> allocate(bool is_primary = true);

private:
    VkCommandBuffer _allocate(bool is_primary = true);

    void push_recycled_cmd(VkCommandBuffer cmd, bool is_primary) {
        (is_primary ? m_recycled_primary_buffers : m_recycled_secondary_buffers).push_back(cmd);
    }

private:
    VkCommandPool m_command_pool;
    std::deque<VkCommandBuffer> m_recycled_primary_buffers, m_recycled_secondary_buffers;
};

} // namespace vke
