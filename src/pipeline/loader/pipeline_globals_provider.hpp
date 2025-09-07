#pragma once

#include <memory>
#include <vke/fwd.hpp>
#include <unordered_map>
#include <string>

#include "../../builders/vertex_input_builder.hpp"
#include "../../isubpass.hpp"

namespace vke{

class ShaderCompiler;

class PipelineGlobalsProvider{
public:
    std::unordered_map<std::string, std::unique_ptr<vke::ISubpass>> subpasses;
    std::unordered_map<std::string, std::unique_ptr<VertexInputDescriptionBuilder>> vertex_input_descriptions;
    std::unordered_map<std::string, VkDescriptorSetLayout> set_layouts;
    std::unique_ptr<ShaderCompiler> shader_compiler;

    void set_globals(class GPipelineBuilder& builder, class PipelineDescription* description) const;
    void set_globals(class PipelineBuilderBase& builder, class PipelineDescription* description) const;

    PipelineGlobalsProvider();
    ~PipelineGlobalsProvider();
};

}