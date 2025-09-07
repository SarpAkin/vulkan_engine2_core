#pragma once

// this header is designed to be only included from shader_compiler.cpp

#include <shaderc/shaderc.h>
#include <vke/util.hpp>

namespace vke {

static shaderc_shader_kind inferShaderType(const std::string& filePath) {
    if (filePath.ends_with(".frag") || filePath.ends_with(".fsh")) return shaderc_glsl_fragment_shader;
    if (filePath.ends_with(".vert") || filePath.ends_with(".vsh")) return shaderc_glsl_vertex_shader;
    if (filePath.ends_with(".geom")) return shaderc_glsl_geometry_shader;
    if (filePath.ends_with(".comp")) return shaderc_glsl_compute_shader;
    if (filePath.ends_with(".tesc")) return shaderc_glsl_tess_control_shader;
    if (filePath.ends_with(".tese")) return shaderc_glsl_tess_evaluation_shader;
    if (filePath.ends_with(".mesh")) return shaderc_glsl_mesh_shader;
    if (filePath.ends_with(".task")) return shaderc_glsl_task_shader;

    throw std::runtime_error("Unknown shader file extension: " + filePath);
}

static VkShaderStageFlagBits convert_to_vk(shaderc_shader_kind kind) {
    switch (kind) {
    case shaderc_glsl_vertex_shader: return VK_SHADER_STAGE_VERTEX_BIT;
    case shaderc_glsl_fragment_shader: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case shaderc_glsl_compute_shader: return VK_SHADER_STAGE_COMPUTE_BIT;
    case shaderc_glsl_geometry_shader: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case shaderc_glsl_tess_control_shader: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case shaderc_glsl_tess_evaluation_shader: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case shaderc_glsl_mesh_shader: return VK_SHADER_STAGE_MESH_BIT_EXT;
    case shaderc_glsl_task_shader: return VK_SHADER_STAGE_TASK_BIT_EXT;
    default: THROW_ERROR("failed to convert shaderc_shader_kind: %d to VkShaderStageFlags", kind);
    }
}

} // namespace vke