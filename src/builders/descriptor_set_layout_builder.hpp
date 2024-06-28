#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vke {

class DescriptorSetLayoutBuilder {
public:
    inline DescriptorSetLayoutBuilder& add_ubo(VkShaderStageFlags stage, uint32_t count = 1) {
        add_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage, count);
        return *this;
    }
    inline DescriptorSetLayoutBuilder& add_ssbo(VkShaderStageFlags stage, uint32_t count = 1) {
        add_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stage, count);
        return *this;
    }
    inline DescriptorSetLayoutBuilder& add_dyn_ubo(VkShaderStageFlags stage, uint32_t count = 1) {
        add_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, stage, count);
        return *this;
    }
    inline DescriptorSetLayoutBuilder& add_image_sampler(VkShaderStageFlags stage, uint32_t count = 1) {
        add_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, count);
        return *this;
    }
    inline DescriptorSetLayoutBuilder& add_input_attachment(VkShaderStageFlags stage, uint32_t count = 1) {
        add_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, stage, count);
        return *this;
    }

    static void cleanup_layouts();

    VkDescriptorSetLayout build();

    void add_binding(VkDescriptorType type, VkShaderStageFlags stage, uint32_t count);
private:

    std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

} // namespace vke