#include "enum_parsers.hpp"

#include <vke/util.hpp>

namespace vke {

VkShaderStageFlagBits infer_shader_stage(const std::string& filepath) {
    if (filepath.ends_with(".vert")) return VK_SHADER_STAGE_VERTEX_BIT;
    if (filepath.ends_with(".frag")) return VK_SHADER_STAGE_FRAGMENT_BIT;
    if (filepath.ends_with(".comp")) return VK_SHADER_STAGE_COMPUTE_BIT;
    if (filepath.ends_with(".tesc")) return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (filepath.ends_with(".tese")) return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if (filepath.ends_with(".geom")) return VK_SHADER_STAGE_GEOMETRY_BIT;
    if (filepath.ends_with(".mesh")) return VK_SHADER_STAGE_MESH_BIT_EXT;
    if (filepath.ends_with(".task")) return VK_SHADER_STAGE_TASK_BIT_EXT;

    // Default to vertex shader if the extension is not recognized
    return VK_SHADER_STAGE_VERTEX_BIT;
}

VkCompareOp parse_depth_op(const std::string& str) {
    if (str == "NEVER") return VK_COMPARE_OP_NEVER;
    if (str == "LESS") return VK_COMPARE_OP_LESS;
    if (str == "EQUAL") return VK_COMPARE_OP_EQUAL;
    if (str == "LESS_OR_EQUAL") return VK_COMPARE_OP_LESS_OR_EQUAL;
    if (str == "GREATER") return VK_COMPARE_OP_GREATER;
    if (str == "NOT_EQUAL") return VK_COMPARE_OP_NOT_EQUAL;
    if (str == "GREATER_OR_EQUAL") return VK_COMPARE_OP_GREATER_OR_EQUAL;
    if (str == "ALWAYS") return VK_COMPARE_OP_ALWAYS;

    return VK_COMPARE_OP_LESS; // Default to LESS
}

VkCullModeFlagBits parse_cull_mode(const std::string& str) {
    if (str == "NONE") return VK_CULL_MODE_NONE;
    if (str == "FRONT") return VK_CULL_MODE_FRONT_BIT;
    if (str == "BACK") return VK_CULL_MODE_BACK_BIT;
    if (str == "FRONT_AND_BACK") return VK_CULL_MODE_FRONT_AND_BACK;
    return VK_CULL_MODE_NONE; // Default to NONE
}

VkPrimitiveTopology parse_topology_mode(const std::string& str) {
    if (str == "POINT_LIST") return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    if (str == "LINE_LIST") return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    if (str == "LINE_STRIP") return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    if (str == "TRIANGLE_LIST") return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    if (str == "TRIANGLE_STRIP") return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    if (str == "TRIANGLE_FAN") return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    if (str == "LINE_LIST_WITH_ADJACENCY") return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
    if (str == "LINE_STRIP_WITH_ADJACENCY") return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
    if (str == "TRIANGLE_LIST_WITH_ADJACENCY") return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
    if (str == "TRIANGLE_STRIP_WITH_ADJACENCY") return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
    if (str == "PATCH_LIST") return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Default to TRIANGLE_LIST
}

VkPolygonMode parse_polygon_mode(const std::string& str) {
    if (str == "FILL") return VK_POLYGON_MODE_FILL;
    if (str == "LINE") return VK_POLYGON_MODE_LINE;
    if (str == "POINT") return VK_POLYGON_MODE_POINT;

    return VK_POLYGON_MODE_FILL;
}

VkShaderStageFlags parse_shader_stage(const std::string& str) {
    if (str == "VERTEX") return VK_SHADER_STAGE_VERTEX_BIT;
    if (str == "FRAGMENT" || str == "FRAG") return VK_SHADER_STAGE_FRAGMENT_BIT;
    if (str == "COMPUTE" || str == "COMP") return VK_SHADER_STAGE_COMPUTE_BIT;
    if (str == "GEOMETRY" || str == "GEOM") return VK_SHADER_STAGE_GEOMETRY_BIT;
    if (str == "TESSELLATION_CONTROL" || str == "TESC") return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (str == "TESSELLATION_EVALUATION" || str == "TESE") return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if (str == "MESH") return VK_SHADER_STAGE_MESH_BIT_EXT;
    if (str == "TASK") return VK_SHADER_STAGE_TASK_BIT_EXT;
    if (str == "RAYGEN") return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    if (str == "ANY_HIT") return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    if (str == "CLOSEST_HIT") return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    if (str == "MISS") return VK_SHADER_STAGE_MISS_BIT_KHR;
    if (str == "INTERSECTION") return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    if (str == "CALLABLE") return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    if (str == "ALL_GRAPHICS") return VK_SHADER_STAGE_ALL_GRAPHICS;
    if (str == "ALL") return VK_SHADER_STAGE_ALL;

    THROW_ERROR("shader stage string \"%s\" is invalid", str.c_str());
}

VkDescriptorType parse_descriptor_type(const std::string& str) {
    if (str == "SSBO" || str == "STORAGE_BUFFER") return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    if (str == "UBO" || str == "UNIFORM_BUFFER") return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    if (str == "SSBOD" || str == "STORAGE_BUFFER_DYNAMIC") return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    if (str == "UBOD" || str == "UNIFORM_BUFFER_DYNAMIC") return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    if (str == "SAMPLER") return VK_DESCRIPTOR_TYPE_SAMPLER;
    if (str == "COMBINED_IMAGE_SAMPLER") return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    if (str == "SAMPLED_IMAGE") return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    if (str == "STORAGE_IMAGE") return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    if (str == "UNIFORM_TEXEL_BUFFER" || str == "TBO") return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    if (str == "STORAGE_TEXEL_BUFFER" || str == "IMAGE_TBO") return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

    if (str == "INPUT_ATTACHMENT") return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

    if (str == "INLINE_UNIFORM_BLOCK") return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
    if (str == "ACCELERATION_STRUCTURE") return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    if (str == "MUTABLE") return VK_DESCRIPTOR_TYPE_MUTABLE_EXT;

    THROW_ERROR("descriptor type string \"%s\" is invalid", str.c_str());
}
} // namespace vke