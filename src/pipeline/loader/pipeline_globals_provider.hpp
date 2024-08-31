#pragma once

#include <memory>
#include <vke/fwd.hpp>
#include <unordered_map>
#include <string>

#include "../../builders/vertex_input_builder.hpp"

namespace vke{

class PipelineGlobalsProvider{
public:
    std::unordered_map<std::string, std::unique_ptr<vke::ISubpass>> subpasses;
    std::unordered_map<std::string, std::unique_ptr<VertexInputDescriptionBuilder>> vertex_input_descriptions;
    std::unordered_map<std::string, VkDescriptorSetLayout> set_layouts;

    void set_globals(class GPipelineBuilder& builder, class PipelineDescription* description) const;
    void set_globals(class PipelineBuilderBase& builder, class PipelineDescription* description) const;
};

}