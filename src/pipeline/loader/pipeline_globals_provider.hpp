#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vke/fwd.hpp>

#include "../../builders/vertex_input_builder.hpp"
#include "../../isubpass.hpp"

namespace vke {

class ShaderCompiler;
class PipelineDescription;
class GPipelineBuilder;
class PipelineBuilderBase;

class PipelineGlobalsProvider {
public:
    std::unordered_map<std::string, std::unique_ptr<vke::ISubpass>> subpasses;
    std::unordered_map<std::string, std::unique_ptr<VertexInputDescriptionBuilder>> vertex_input_descriptions;
    std::unordered_map<std::string, VkDescriptorSetLayout> set_layouts;
    std::unique_ptr<ShaderCompiler> shader_compiler;

    void set_globals(GPipelineBuilder& builder, const   PipelineDescription* description) const;
    void set_globals(PipelineBuilderBase& builder, const PipelineDescription* description) const;

    PipelineGlobalsProvider();
    ~PipelineGlobalsProvider();
};

} // namespace vke