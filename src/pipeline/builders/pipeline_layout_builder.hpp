#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <vke/fwd.hpp>

namespace vke{

class PipelineLayoutBuilder {
public:
    template <typename T>
    PipelineLayoutBuilder& add_push_constant(VkShaderStageFlags stage) {
        m_push_constants.push_back(VkPushConstantRange{
            .stageFlags = stage,
            .offset     = 0,
            .size       = sizeof(T),
        });

        return *this;
    }

    PipelineLayoutBuilder& add_push_constant(VkShaderStageFlags stage, u32 size) {
        m_push_constants.push_back(VkPushConstantRange{
            .stageFlags = stage,
            .offset     = 0,
            .size       = size,
        });

        return *this;
    }

    inline PipelineLayoutBuilder& add_set_layout(VkDescriptorSetLayout layout) {
        m_set_layouts.push_back(layout);

        return *this;
    }

    VkPipelineLayout build();

private:
    std::vector<VkPushConstantRange> m_push_constants;
    std::vector<VkDescriptorSetLayout> m_set_layouts;
};

}