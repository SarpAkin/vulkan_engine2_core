#pragma once

#include <vke/fwd.hpp>

#include <vulkan/vulkan.h>

#include <json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

#include "enum_parsers.hpp"

namespace vke {

using json = nlohmann::json;

class PipelineDescription {
public:
    std::string name;
    std::string renderpass;
    std::string vertex_input;
    std::vector<std::string> shader_files;

    std::unordered_map<std::string, std::string> compiler_definitions;

    std::unordered_map<std::string, int> set_layouts;

    VkPolygonMode polygon_mode        = VK_POLYGON_MODE_FILL;
    VkPrimitiveTopology topology_mode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlagBits cull_mode      = VK_CULL_MODE_NONE;
    bool depth_test                   = true;
    bool depth_write                  = true;

    //not serialized
    std::string file_path;

    void load(const json& json) {
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
    }
};

class PipelineFile {
public:
    std::vector<PipelineDescription> pipelines;

    void load(const json& json) {
        for(const auto& pipeline : json.value("pipelines",json::array())){
            PipelineDescription description;
            description.load(pipeline);
            pipelines.push_back(std::move(description));
        }
    }
};

} // namespace vke