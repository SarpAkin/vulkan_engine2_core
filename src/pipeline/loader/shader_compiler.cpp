#include "shader_compiler.hpp"

#include <filesystem>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <string>

#include <vke/util.hpp>

#include <vulkan/vulkan.h>

namespace vke {

class ShadercIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    ShadercIncluder(ArenaAllocator* _arena) {
        m_arena = _arena;
    }

    ArenaAllocator* m_arena;

    shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override {
        if (type == shaderc_include_type_standard) return nullptr;

        fs::path path = fs::path(requesting_source).parent_path() / requested_source;

        auto content = read_file(m_arena, path.c_str());

        size_t pathc_len;
        const char* pathc = m_arena->create_str_copy(path.c_str(), &pathc_len);

        return m_arena->create_copy(shaderc_include_result{
            .source_name        = pathc,
            .source_name_length = pathc_len,
            .content            = content.begin(),
            .content_length     = content.size(),
        });
    }

    void ReleaseInclude(shaderc_include_result* data) override {
    }
};

void add_shader_kind_macro_def(shaderc::CompileOptions& options, shaderc_shader_kind kind) {
    switch (kind) {
    case shaderc_vertex_shader:
        options.AddMacroDefinition("VERTEX_SHADER");
        break;
    case shaderc_fragment_shader:
        options.AddMacroDefinition("FRAGMENT_SHADER");
        break;
    case shaderc_compute_shader:
        options.AddMacroDefinition("COMPUTE_SHADER");
        break;
    case shaderc_geometry_shader:
        options.AddMacroDefinition("GEOMETRY_SHADER");
        break;
    case shaderc_tess_control_shader:
        options.AddMacroDefinition("TESS_CONTROL_SHADER");
        break;
    case shaderc_tess_evaluation_shader:
        options.AddMacroDefinition("TESS_EVALUATION_SHADER");
        break;
    case shaderc_task_shader:
        options.AddMacroDefinition("TASK_SHADER");
        break;
    case shaderc_mesh_shader:
        options.AddMacroDefinition("MESH_SHADER");
        break;
    case shaderc_raygen_shader:
        options.AddMacroDefinition("RAYGEN_SHADER");
        break;
    case shaderc_anyhit_shader:
        options.AddMacroDefinition("ANY_HIT_SHADER");
        break;
    case shaderc_closesthit_shader:
        options.AddMacroDefinition("CLOSEST_HIT_SHADER");
        break;
    case shaderc_miss_shader:
        options.AddMacroDefinition("MISS_SHADER");
        break;
    case shaderc_intersection_shader:
        options.AddMacroDefinition("INTERSECTION_SHADER");
        break;
    case shaderc_callable_shader:
        options.AddMacroDefinition("CALLABLE_SHADER");
        break;

    default:
        break;
    }
}

shaderc_shader_kind inferShaderType(const std::string& filePath) {
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

VkShaderStageFlagBits convert_to_vk(shaderc_shader_kind kind) {
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

CompiledShader compile_glsl(const std::string& file_path, const std::unordered_map<std::string, std::string>& flags) {
    ArenaAllocator arena;

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetSpirv(shaderc_spirv_version_1_5);

    // options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetIncluder(std::make_unique<ShadercIncluder>(&arena));

    options.SetGenerateDebugInfo();

    auto source = read_file(&arena, file_path.c_str());

    for (auto& [name, definition] : flags) {
        options.AddMacroDefinition(name, definition);
    }

    auto type = inferShaderType(file_path);

    add_shader_kind_macro_def(options, type);

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source.begin(), source.size(), type, file_path.c_str(), "main", options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error("Shader compilation failed: " + std::string(result.GetErrorMessage()));
    }

    return CompiledShader{
        .spv = std::vector<uint32_t>(result.begin(), result.end()),
        .stage = convert_to_vk(type),    
    };
}

std::vector<CompiledShader> compile_shaders(PipelineDescription* description) {
    std::vector<CompiledShader> compiled_shaders;

    auto base_path = fs::path(description->file_path).parent_path();

    for (auto& shader : description->shader_files) {
        compiled_shaders.push_back(compile_glsl(base_path / shader, description->compiler_definitions));
    }

    return compiled_shaders;
}

} // namespace vke
