#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "../fwd.hpp"
#include "../util/arena_alloc.hpp"

struct SpvReflectShaderModule;
struct SpvReflectDescriptorBinding;

namespace vke {


class PipelineReflection {
public:
    struct LayoutBuild {
        VkPipelineLayout layout;
        std::vector<VkDescriptorSetLayout> dset_layouts;
        VkShaderStageFlagBits push_stages;
    };

    PipelineReflection() {}

    VkShaderStageFlagBits add_shader_stage(std::span<const u32> spirv);

    LayoutBuild build_pipeline_layout() const;
    // std::unique_ptr<BufferReflection> reflect_buffer(u32 set, u32 binding) const;

private:
    struct ShaderStage {
        std::span<u32> spirv;
        VkShaderStageFlagBits stage;
        SpvReflectShaderModule* module;
    };

    std::vector<std::pair<SpvReflectDescriptorBinding*, const ShaderStage*>> find_bindings(u32 set, u32 binding) const;

    ArenaAllocator m_alloc;

    std::vector<ShaderStage> m_shaders;
};
} // namespace vke