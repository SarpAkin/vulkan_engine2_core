#pragma once

#include <vke/fwd.hpp>

#include <string>
#include <unordered_map>
#include <vector>

#include <cereal/cereal.hpp>

#include "vulkan_enums_cereal.hpp"

namespace vke {

class PipelineDescription {
public:
    std::string name;
    std::string renderpass;
    std::string vertex_input;
    std::vector<std::string> shader_files;

    std::unordered_map<std::string, std::string> compiler_definitions;

    VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
    VkPrimitiveTopology topology_mode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlagBits cull_mode = VK_CULL_MODE_NONE;
    bool depth_test  = true;
    bool depth_write = true;

    template <class Archive>
    void serialize(Archive& archive) {
        archive( //
            CEREAL_NVP(name),
            CEREAL_NVP(renderpass),
            CEREAL_NVP(vertex_input),
            CEREAL_NVP(shader_files),
            CEREAL_NVP(compiler_definitions),
            CEREAL_NVP(polygon_mode),
            CEREAL_NVP(topology_mode),
            CEREAL_NVP(cull_mode),
            CEREAL_NVP(depth_test),
            CEREAL_NVP(depth_write) //
        );
    }
};

class PipelineFile {
public:
    std::vector<PipelineDescription> pipelines;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(pipelines));
    }
};



// void foo() {
//     PipelineFile pipelineFile;

//     std::ifstream is("pipeline.json");
//     cereal::JSONInputArchive archive(is);
//     archive(CEREAL_NVP(pipelineFile));
// }

} // namespace vke