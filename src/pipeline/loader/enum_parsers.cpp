#include "enum_parsers.hpp"

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
} // namespace vke