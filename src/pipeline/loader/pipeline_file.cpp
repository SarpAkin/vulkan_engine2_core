#include "pipeline_file.hpp"

#include "enum_parsers.hpp"

#include <vke/util.hpp>

namespace vke {

void PipelineDescription::load(const json& json) {
    name         = json.value("name", "");
    renderpass   = json.value("renderpass", "");
    vertex_input = json.value("vertex_input", "");

    shader_files = json.value("shader_files", std::vector<std::string>{});

    auto defs = json.value("compiler_definitions", json::object());
    for (auto it = defs.begin(); it != defs.end(); ++it) {
        compiler_definitions[it.key()] = it.value().get<std::string>();
    }

    auto layouts = json.value("set_layouts", json::object());
    for (auto it = layouts.begin(); it != layouts.end(); ++it) {
        set_layouts[it.key()] = it.value().get<int>();
    }

    polygon_mode  = parse_polygon_mode(json.value("polygon_mode", "FILL"));
    topology_mode = parse_topology_mode(json.value("topology_mode", "TRIANGLE_LIST"));
    cull_mode     = parse_cull_mode(json.value("cull_mode", "NONE"));
    depth_test    = json.value("depth_test", true);
    depth_write   = json.value("depth_write", true);
    depth_op      = parse_depth_op(json.value("depth_op", "LESS_OR_EQUAL"));
}

void PipelineFile::load(const json& json) {
    for (const auto& pipeline : json.value("pipelines", json::array())) {
        auto description = std::make_shared<PipelineDescription>();
        description->load(pipeline);
        pipelines.push_back(std::move(description));
    }

    for (const auto& mpipeline : json.value("multi_pipelines", json::array())) {
        MultiPipelineDescription description;
        description.load(mpipeline);
        multi_pipelines.push_back(std::move(description));
    }
}

void MultiPipelineDescription::load(const json& top) {
    name = top.value("name", std::string());

    pipelines = map_vec(top.value("pipelines", json::array()), [&](const json& j) {
        return j.get<std::string>();
    });
}
} // namespace vke