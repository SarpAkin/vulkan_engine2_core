#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "vk_resource.hpp"

namespace vke {

class GPipelineBuilder;
class CPipelineBuilder;
class PipelineReflection;

class Pipeline : public Resource {
    friend GPipelineBuilder;
    friend CPipelineBuilder;

public:
    struct PipelineData {
        VkShaderStageFlagBits push_stages;
        std::vector<VkDescriptorSetLayout> dset_layouts;
    };

    Pipeline(VkPipeline pipeline, VkPipelineLayout layout, VkPipelineBindPoint bindpoint);
    ~Pipeline();

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_layout; }
    VkPipelineBindPoint bindpoint() const { return m_bindpoint; }

    const PipelineData& data() const { return m_data; }
    VkDescriptorSetLayout get_descriptor_layout(u32 index) const { return m_data.dset_layouts[index]; }

    const PipelineReflection* get_reflection() const { return m_reflection.get(); }

private:
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    VkPipelineBindPoint m_bindpoint;
    PipelineData m_data;
    std::unique_ptr<PipelineReflection> m_reflection;
};

} // namespace vke