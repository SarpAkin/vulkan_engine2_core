#pragma once

#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "../common.hpp"
#include "../fwd.hpp"
#include "../image.hpp"

namespace vke {

class DescriptorSetBuilder {
public:
    inline DescriptorSetBuilder& add_ubo(IBufferSpan* buffer, VkShaderStageFlags stage) { return add_buffer(buffer, stage, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); }
    inline DescriptorSetBuilder& add_ssbo(IBufferSpan* buffer, VkShaderStageFlags stage) { return add_buffer(buffer, stage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); }
    inline DescriptorSetBuilder& add_dyn_ubo(IBufferSpan* buffer, VkShaderStageFlags stage) { return add_buffer(buffer, stage, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC); }

    inline DescriptorSetBuilder& add_ssbo(std::span<IBufferSpan*> buffers, VkShaderStageFlags stage) { return add_buffers(buffers, stage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); }

    inline DescriptorSetBuilder& add_image_sampler(IImageView* image, VkImageLayout layout, VkSampler sampler, VkShaderStageFlags stage) {
        IImageView* images[] = {image};
        return add_images(images, layout, sampler, stage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    DescriptorSetBuilder& add_image_samplers(std::span<Image*> images, VkImageLayout layout, VkSampler sampler, VkShaderStageFlags stage);

    inline DescriptorSetBuilder& add_image_samplers(std::span<IImageView*> images, VkImageLayout layout, VkSampler sampler, VkShaderStageFlags stage) {
        return add_images(images, layout, sampler, stage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    inline DescriptorSetBuilder& add_storage_image(IImageView* image, VkImageLayout layout, VkShaderStageFlags stage) {
        IImageView* images[] = {image};
        return add_images(images, layout, nullptr, stage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }

    VkDescriptorSet build(DescriptorPool* pool, VkDescriptorSetLayout layout);
    

private:
    inline DescriptorSetBuilder& add_buffer(IBufferSpan* buffer, VkShaderStageFlags stage, VkDescriptorType type) {
        IBufferSpan* buffers[] = {buffer};
        return add_buffers(buffers, stage, type);
    }

    DescriptorSetBuilder& add_buffers(std::span<IBufferSpan*> buffers, VkShaderStageFlags stage, VkDescriptorType type);
    DescriptorSetBuilder& add_images(std::span<IImageView*> images, VkImageLayout layout, VkSampler sampler, VkShaderStageFlags stage, VkDescriptorType type);

private:
    struct ImageBinding {
        std::vector<VkDescriptorImageInfo> image_infos;
        u32 binding;
        VkDescriptorType type;
    };
    std::vector<ImageBinding> m_image_bindings;

    struct BufferBinding {
        std::vector<VkDescriptorBufferInfo> buffer_infos;
        u32 binding;
        VkDescriptorType type;
    };
    std::vector<BufferBinding> m_buffer_bindings;

    u32 m_binding_counter = 0;
};

} // namespace vke
