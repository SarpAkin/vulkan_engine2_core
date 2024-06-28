#include "pipeline_reflection.hpp"

// #include <fmt/core.h>

#include <spirv_reflect.h>

#include "../builders/descriptor_set_layout_builder.hpp"
#include "../builders/pipeline_layout_builder.hpp"

#include "../util/util.hpp"
// #include "buffer_reflection.hpp"
#include "spv_reflect_util.hpp"

namespace vke {

VkShaderStageFlagBits PipelineReflection::add_shader_stage(std::span<const u32> _spirv) {
    auto spirv = m_alloc.create_copy(_spirv);

    auto* module = m_alloc.alloc<SpvReflectShaderModule>();

    SPV_CHECK(spvReflectCreateShaderModule(spirv.size_bytes(), spirv.data(), module));

    VkShaderStageFlags stage = convert_to_vk(module->shader_stage);

    m_shaders.push_back(ShaderStage{
        .spirv  = m_alloc.create_copy(spirv),
        .stage  = static_cast<VkShaderStageFlagBits>(stage),
        .module = module,
    });

    return static_cast<VkShaderStageFlagBits>(stage);
}

PipelineReflection::LayoutBuild PipelineReflection::build_pipeline_layout() const {
    struct DescriptorSetInfo {
        struct BindingInfo {
            VkDescriptorType type;
            VkShaderStageFlags stage;
            u32 count;
        };
        std::span<BindingInfo> bindings;
    } set_infos[4] = {};

    u32 push_size = UINT32_MAX;
    VkShaderStageFlags push_stage;

    for (auto& shader : m_shaders) {
        auto* module = shader.module;
        for (auto& set : std::span(module->descriptor_sets, module->descriptor_set_count)) {
            auto& set_info = set_infos[set.set];

            if (set_info.bindings.data() == nullptr) {
                set_info.bindings = ALLOCA_ARR(DescriptorSetInfo::BindingInfo, set.binding_count);
                memset(set_info.bindings.data(), 0, set_info.bindings.size_bytes());
            }

            for (auto* binding : std::span(set.bindings, set.binding_count)) {
                auto& reflection_binding = set.bindings[binding->binding];

                auto& binding_info = set_info.bindings[reflection_binding->binding];

                binding_info.type = convert_to_vk(reflection_binding->descriptor_type);
                binding_info.stage |= shader.stage;
                binding_info.count = reflection_binding->count;
            }
        }

        if (push_size == UINT32_MAX && module->push_constant_block_count > 0) {
            push_size  = module->push_constant_blocks[0].size;
            push_stage = shader.stage;
        }
    }

    std::vector<VkDescriptorSetLayout> dset_layouts;

    for (auto& set_info : std::span(set_infos)) {
        if (set_info.bindings.empty()) break;

        DescriptorSetLayoutBuilder builder;
        for (auto& binding : set_info.bindings) {
            builder.add_binding(binding.type, binding.stage, binding.count);
        }

        dset_layouts.push_back(builder.build());
    }

    PipelineLayoutBuilder p_builder;
    if (push_size != UINT32_MAX) {
        p_builder.add_push_constant(push_stage, push_size);
    }
    for (auto& set_layout : dset_layouts) {
        p_builder.add_set_layout(set_layout);
    }
    VkPipelineLayout layout = p_builder.build();
    dset_layouts.resize(4);

    return LayoutBuild{
        .layout       = layout,
        .dset_layouts = dset_layouts,
        .push_stages  = (VkShaderStageFlagBits)push_stage,
    };
}

// static std::unique_ptr<BufferReflection> reflect_binding(SpvReflectDescriptorBinding* binding, VkShaderStageFlagBits flags) {
//     if (binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER && binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) return nullptr;
//     auto* block = &binding->block;

//     return std::make_unique<BufferReflection>(block, flags);
// }

// std::unique_ptr<BufferReflection> PipelineReflection::reflect_buffer(u32 nset, u32 nbinding) const {
//     auto bindings = find_bindings(nset, nbinding);

//     if (bindings.size() == 0) return nullptr;

//     VkShaderStageFlags stages;
//     for (auto& [binding, shader] : bindings) {
//         stages |= shader->stage;
//     }

//     return reflect_binding(bindings[0].first, static_cast<VkShaderStageFlagBits>(stages));
// }

std::vector<std::pair<SpvReflectDescriptorBinding*, const PipelineReflection::ShaderStage*>> PipelineReflection::find_bindings(u32 nset, u32 nbinding) const {
    std::vector<std::pair<SpvReflectDescriptorBinding*, const ShaderStage*>> found_bindings;

    for (auto& shader : m_shaders) {
        SpvReflectShaderModule* module = shader.module;

        for (auto& set : std::span(module->descriptor_sets, module->descriptor_set_count)) {
            if (set.set != nset) continue;

            for (auto& binding : std::span(set.bindings, set.binding_count)) {
                if (binding->binding != nbinding) continue;

                found_bindings.push_back(std::pair(binding, &shader));
            }
        }
    }

    return found_bindings;
}

} // namespace vke
