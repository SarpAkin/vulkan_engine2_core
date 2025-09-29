#include "pipeline_globals_provider.hpp"

#include "../builders/pipeline_builder.hpp"
#include "shader_compiler/shader_compiler.hpp"

#include "pipeline_file.hpp"

#include "../../isubpass.hpp" // IWYU pragma: KEEP

#include <vke/util.hpp>

namespace vke {

void PipelineGlobalsProvider::set_globals(GPipelineBuilder& builder, const PipelineDescription* description) const {
    if (!description->renderpass.empty()) {
        auto it = this->subpasses.find(description->renderpass);
        if (it == this->subpasses.end()) THROW_ERROR("renderpass %s was not found in description %s", description->renderpass.c_str(), description->name.c_str());

        builder.set_renderpass(it->second.get());
    } else {
        THROW_ERROR("renderpass must not be empty in pipeline description %s", description->name.c_str());
    }

    if (!description->vertex_input.empty()) {
        auto it = this->vertex_input_descriptions.find(description->vertex_input);
        if (it == this->vertex_input_descriptions.end()) THROW_ERROR("vertex input description %s not found", description->vertex_input.c_str());

        builder.set_vertex_input(it->second.get());
    }

    set_globals(static_cast<PipelineBuilderBase&>(builder), description);
}

void PipelineGlobalsProvider::set_globals(PipelineBuilderBase& builder, const PipelineDescription* description) const {
    for (auto& [name, set_index] : description->set_layouts) {
        auto it = set_layouts.find(name);
        if (it == set_layouts.end()) THROW_ERROR("set layout %s not found", name.c_str());

        builder.set_descriptor_set_layout(set_index, it->second);
    }
}

PipelineGlobalsProvider::PipelineGlobalsProvider() {
    shader_compiler = std::make_unique<ShaderCompiler>();
}

PipelineGlobalsProvider::~PipelineGlobalsProvider() {}
} // namespace vke