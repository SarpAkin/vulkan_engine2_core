#pragma once

#include <cassert>
#include <vector>

#include "../common.hpp"

#include <vulkan/vulkan_core.h>

namespace vke {

namespace imp {
template <typename T>
struct Type {};

constexpr inline VkFormat get_format(u32 count, Type<f32>) {
    constexpr VkFormat formats[] = {VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};
    return formats[count - 1];
}

constexpr inline VkFormat get_format(u32 count, Type<i32>) {
    constexpr VkFormat formats[] = {VK_FORMAT_R32_SINT, VK_FORMAT_R32G32_SINT, VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32A32_SINT};
    return formats[count - 1];
}

constexpr inline VkFormat get_format(u32 count, Type<u32>) {
    constexpr VkFormat formats[] = {VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32A32_UINT};
    return formats[count - 1];
}

} // namespace imp

class VertexInputDescriptionBuilder {
public:
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;

    VkPipelineVertexInputStateCreateInfo get_info() const {
        return VkPipelineVertexInputStateCreateInfo{
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = static_cast<u32>(bindings.size()),
            .pVertexBindingDescriptions      = bindings.data(),
            .vertexAttributeDescriptionCount = static_cast<u32>(attributes.size()),
            .pVertexAttributeDescriptions    = attributes.data(),
        };
    }

    template <typename T>
    void push_binding(VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX) {
        VkVertexInputBindingDescription binding = {};
        binding.binding                         = bindings.size();
        binding.stride                          = sizeof(T);
        binding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

        bindings.push_back(binding);

        m_offset_counter = 0;
    }

    template <typename T>
    void push_attribute(u32 count) {
        assert(bindings.size() != 0 && "a binding must be pushed first");
        assert(count <= 4 && count != 0 && "count must be between 1-4");

        attributes.push_back(VkVertexInputAttributeDescription{
            .location = static_cast<u32>(attributes.size()),
            .binding  = static_cast<u32>(bindings.size() - 1),
            .format   = imp::get_format(count, imp::Type<T>{}),
            .offset   = m_offset_counter,
        });

        m_offset_counter += sizeof(T) * count;
    }

private:
    u32 m_offset_counter = 0;
};

} // namespace vke