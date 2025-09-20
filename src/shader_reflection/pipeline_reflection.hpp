#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "../fwd.hpp"
#include "../util/arena_alloc.hpp"

struct SpvReflectShaderModule;
struct SpvReflectDescriptorBinding;
struct SpvReflectBlockVariable;

namespace vke {

class PipelineReflection {
public:
    struct LayoutBuild {
        VkPipelineLayout layout;
        std::vector<VkDescriptorSetLayout> dset_layouts;
        VkShaderStageFlagBits push_stages;
    };

    PipelineReflection() {}

    void set_descriptor_layout(int set, VkDescriptorSetLayout layout);

    VkShaderStageFlagBits add_shader_stage(std::span<const u32> spirv);

    LayoutBuild build_pipeline_layout() const;
    // std::unique_ptr<BufferReflection> reflect_buffer(u32 set, u32 binding) const;

    int determine_tesselation_path_control_points() const;

private:
    void check_for_autopadding(SpvReflectDescriptorBinding* binding) const;
    void check_for_autopadding_in_block(SpvReflectBlockVariable* block) const;

    VkDescriptorSetLayout get_set_layout(int index) const;

private:
    struct ShaderStage {
        std::span<u32> spirv;
        VkShaderStageFlagBits stage;
        SpvReflectShaderModule* module;
    };

private:
    const ShaderStage* find_shader_stage(VkShaderStageFlagBits stage) const;

    std::vector<VkDescriptorSetLayout> m_layouts;

    std::vector<std::pair<SpvReflectDescriptorBinding*, const ShaderStage*>> find_bindings(u32 set, u32 binding) const;

    ArenaAllocator m_alloc;

    std::vector<ShaderStage> m_shaders;
};
} // namespace vke