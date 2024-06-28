#include "descriptor_set_builder.hpp"

#include <vulkan/vulkan_core.h>

#include "../fwd.hpp"
#include "../util.hpp"

#include "buffer.hpp"
#include "descriptor_pool.hpp"
#include "image.hpp"

namespace vke {

DescriptorSetBuilder& DescriptorSetBuilder::add_buffers(std::span<IBufferSpan*> buffers, VkShaderStageFlags stage, VkDescriptorType type) {
    m_buffer_bindings.push_back(BufferBinding{
        .buffer_infos = map_vec(buffers, [&](IBufferSpan* buffer) {
            return VkDescriptorBufferInfo{
                .buffer = buffer->handle(),
                .offset = buffer->byte_offset(),
                .range  = buffer->byte_size(),
            };
        }),
        .binding      = m_binding_counter++,
        .type         = type,
    });

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::add_images(std::span<IImageView*> images, VkImageLayout layout, VkSampler sampler, VkShaderStageFlags stage, VkDescriptorType type) {
    m_image_bindings.push_back(ImageBinding{
        .image_infos = map_vec(images, [&](IImageView* image) {
            return VkDescriptorImageInfo{
                .sampler     = sampler,
                .imageView   = image->view(),
                .imageLayout = layout,
            };
        }),
        .binding     = m_binding_counter++,
        .type        = type,
    });

    return *this;
}

VkDescriptorSet DescriptorSetBuilder::build(DescriptorPool* pool, VkDescriptorSetLayout layout) {

    VkDescriptorSet set = pool->allocate_set(layout);

    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(m_buffer_bindings.size() + m_image_bindings.size());

    for (auto& buffer_binding : m_buffer_bindings) {
        writes.push_back(VkWriteDescriptorSet{
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set,
            .dstBinding      = buffer_binding.binding,
            .descriptorCount = static_cast<u32>(buffer_binding.buffer_infos.size()),
            .descriptorType  = buffer_binding.type,
            .pBufferInfo     = buffer_binding.buffer_infos.data(),
        });
    }

    for (auto& image_binding : m_image_bindings) {
        writes.push_back(VkWriteDescriptorSet{
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set,
            .dstBinding      = image_binding.binding,
            .descriptorCount = static_cast<u32>(image_binding.image_infos.size()),
            .descriptorType  = image_binding.type,
            .pImageInfo      = image_binding.image_infos.data(),
        });
    }

    vkUpdateDescriptorSets(pool->device(), writes.size(), writes.data(), 0, nullptr);

    return set;
}

DescriptorSetBuilder& DescriptorSetBuilder::add_image_samplers(std::span<Image*> images, VkImageLayout layout, VkSampler sampler, VkShaderStageFlags stage) {

    auto span = MAP_VEC_ALLOCA(images, [](Image* image) { return static_cast<IImageView*>(image); }); 

    return add_images(span, layout, sampler, stage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}
} // namespace vke