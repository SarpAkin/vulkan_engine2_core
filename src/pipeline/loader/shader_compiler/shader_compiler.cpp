#include "shader_compiler.hpp"

#include <filesystem>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <string>

#include <vke/util.hpp>

#include <vulkan/vulkan.h>

#include "shaderc_util.hpp"

#include "include_resolver/iinclude_resolver.hpp"
#include "include_resolver/relative_include_resolver.hpp"
#include "include_resolver/library_include_resolver.hpp"

namespace vke {

class ShadercIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    ShadercIncluder(ArenaAllocator* _arena, ShaderCompiler* compiler) {
        m_arena           = _arena;
        m_shader_compiler = compiler;
    }

    std::optional<IGlslIncludeResolver::IncludeResolverReturn> resolve(const IGlslIncludeResolver::IncludeResolveParameters& paremeters) {
        for (auto& includers : m_shader_compiler->get_includers()) {
            try {
                auto ret = includers->resolve_include(m_arena, paremeters);

                if (ret.has_value())
                    return ret;
            } catch (std::exception& ex) {
                LOG_ERROR("caught exception on include resolver: %s\n", ex.what());
            }
        }

        return std::nullopt;
    }

    shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override {

        auto requested  = fs::path(requested_source);
        auto requesting = fs::path(requesting_source);

        auto include_resolve = resolve({
            .requested_source  = requested,
            .requesting_source = requesting,
            .is_relative       = type == shaderc_include_type_relative,
        });

        if (!include_resolve.has_value()) {
            auto error = std::format("#error failed to include \"{}\" from \"{}\"", requested_source, requesting_source);

            return m_arena->create_copy(shaderc_include_result{
                .source_name        = "<failed include>",
                .source_name_length = 17,
                .content            = m_arena->create_str_copy(error.data()),
                .content_length     = error.size(),
            });
        }

        return m_arena->create_copy(shaderc_include_result{
            .source_name        = include_resolve->path.data(),
            .source_name_length = include_resolve->path.size(),
            .content            = include_resolve->content.data(),
            .content_length     = include_resolve->content.size(),
        });
    }

    void ReleaseInclude(shaderc_include_result* data) override {
    }

    ArenaAllocator* m_arena;
    ShaderCompiler* m_shader_compiler;
};

void add_shader_kind_macro_def(shaderc::CompileOptions& options, shaderc_shader_kind kind) {
    options.AddMacroDefinition([&] {
        switch (kind) {
        case shaderc_vertex_shader: return "VERTEX_SHADER";
        case shaderc_fragment_shader: return "FRAGMENT_SHADER";
        case shaderc_compute_shader: return "COMPUTE_SHADER";
        case shaderc_geometry_shader: return "GEOMETRY_SHADER";
        case shaderc_tess_control_shader: return "TESS_CONTROL_SHADER";
        case shaderc_tess_evaluation_shader: return "TESS_EVALUATION_SHADER";
        case shaderc_task_shader: return "TASK_SHADER";
        case shaderc_mesh_shader: return "MESH_SHADER";
        case shaderc_raygen_shader: return "RAY_GEN_SHADER";
        case shaderc_anyhit_shader: return "ANY_HIT_SHADER";
        case shaderc_closesthit_shader: return "CLOSEST_HIT_SHADER";
        case shaderc_miss_shader: return "MISS_SHADER";
        case shaderc_intersection_shader: return "INTERSECTION_SHADER";
        case shaderc_callable_shader: return "CALLABLE_SHADER";
        default: return "";
        }
    }());
}

CompiledShader ShaderCompiler::compile_glsl(const std::string& file_path, const std::unordered_map<std::string, std::string>& flags) {
    ArenaAllocator arena;

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetSpirv(shaderc_spirv_version_1_5);

    // options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetIncluder(std::make_unique<ShadercIncluder>(&arena, this));

    options.SetGenerateDebugInfo();

    auto source = read_file(&arena, file_path.c_str());

    for (auto& [name, definition] : flags) {
        options.AddMacroDefinition(name, definition);
    }

    auto type = inferShaderType(file_path);

    add_shader_kind_macro_def(options, type);

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source.begin(), source.size(), type, file_path.c_str(), "main", options);

    auto status = result.GetCompilationStatus();
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error("Shader compilation failed: " + std::string(result.GetErrorMessage()));
    }

    return CompiledShader{
        .spv   = std::vector<uint32_t>(result.begin(), result.end()),
        .stage = convert_to_vk(type),
    };
}

std::vector<CompiledShader> ShaderCompiler::compile_shaders(PipelineDescription* description) {
    std::vector<CompiledShader> compiled_shaders;

    auto base_path = fs::path(description->file_path).parent_path();

    for (auto& shader : description->shader_files) {
        compiled_shaders.push_back(compile_glsl(base_path / shader, description->compiler_definitions));
    }

    return compiled_shaders;
}

ShaderCompiler::ShaderCompiler() {
    m_include_resolver.push_back(std::make_shared<RelativeIncludeResolver>());
}

ShaderCompiler::~ShaderCompiler() {}

void ShaderCompiler::add_system_include_dir(std::string_view dir) {
    m_include_resolver.push_back(std::make_shared<LibraryIncludeResolver>(dir));
}
} // namespace vke
