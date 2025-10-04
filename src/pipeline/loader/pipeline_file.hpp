#pragma once

#include <vke/fwd.hpp>

#include <vulkan/vulkan.h>

#include <json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

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
    VkCompareOp depth_op              = VK_COMPARE_OP_LESS;
    bool depth_test                   = true;
    bool depth_write                  = true;

    // not serialized
    std::string file_path;
    std::vector<std::string> shader_file_absolute_paths;

    void load(const json& json);
};

class MultiPipelineDescription {
public:
    std::string name;
    std::vector<std::string> pipelines;

    void load(const json& json);
};

class DescriptorSetLayoutDescription {
public:
    struct BindingDescription {
        VkShaderStageFlags stages;
        VkDescriptorType type;
        int count;
    };

    std::string name;
    std::vector<BindingDescription> bindings;

    void load(const json& json);
};

class PipelineFile {
public:
    std::vector<std::shared_ptr<PipelineDescription>> pipelines;
    std::vector<MultiPipelineDescription> multi_pipelines;
    std::vector<DescriptorSetLayoutDescription> descriptor_set_layouts;

    void load(const json& json);
};

} // namespace vke