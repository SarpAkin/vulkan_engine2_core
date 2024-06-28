#include "descriptor_set_layout_builder.hpp"

#include <vulkan/vulkan_core.h>

#include <mutex>
#include <vector>

#include "../vulkan_context.hpp"
#include "../vkutil.hpp"

namespace vke {


static std::vector<VkDescriptorSetLayout> destroy_queue;
static std::mutex destroy_queue_lock; 

void DescriptorSetLayoutBuilder::add_binding(VkDescriptorType type, VkShaderStageFlags stage, uint32_t count) {
    m_bindings.push_back(VkDescriptorSetLayoutBinding{
        .binding         = static_cast<uint32_t>(m_bindings.size()),
        .descriptorType  = type,
        .descriptorCount = count,
        .stageFlags      = stage,
    });
}

VkDescriptorSetLayout DescriptorSetLayoutBuilder::build() {
    VkDescriptorSetLayoutCreateInfo info{
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(m_bindings.size()),
        .pBindings    = m_bindings.data(),
    };

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::get_context()->get_device(), &info, nullptr, &layout));

    //TODO replace with hash & cache
    std::lock_guard<std::mutex> quard(destroy_queue_lock);
    destroy_queue.push_back(layout);

    return layout;
}

void DescriptorSetLayoutBuilder::cleanup_layouts() {
    std::lock_guard<std::mutex> quard(destroy_queue_lock);

    for (auto& layout : destroy_queue) {
        vkDestroyDescriptorSetLayout(VulkanContext::get_context()->get_device(), layout, nullptr);
    }

}
} // namespace vke
