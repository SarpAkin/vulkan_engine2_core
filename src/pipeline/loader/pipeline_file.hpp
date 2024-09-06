#pragma once

#include <vke/fwd.hpp>

#include <vulkan/vulkan.h>

#include <string>
#include <unordered_map>
#include <vector>

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

    
};

class PipelineFile {
public:
    std::vector<PipelineDescription> pipelines;


};

} // namespace vke