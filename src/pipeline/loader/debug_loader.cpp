#include "debug_loader.hpp"

#include "../../builders/pipeline_builder.hpp"

#include "pipeline_file.hpp"
#include "shader_compiler.hpp"

#include "pipeline_file.hpp"
#include "pipeline_globals_provider.hpp"

#include <cereal/archives/json.hpp>

#include <filesystem>
#include <fstream>

#include <vke/util.hpp>

namespace vke {

DebugPipelineLoader::~DebugPipelineLoader() {}

DebugPipelineLoader::DebugPipelineLoader(const char* pipeline_search_path) {
    m_pipeline_search_path = pipeline_search_path;

    load_descriptions();
}

std::unique_ptr<IPipeline> DebugPipelineLoader::load(const char* pipeline_name) {
    auto it = m_pipelines_descriptions.find(pipeline_name);
    if (it == m_pipelines_descriptions.end()) {
        throw std::runtime_error("pipeline not found");
    }

    return load_pipeline(it->second.get());
}

void DebugPipelineLoader::load_descriptions() {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(m_pipeline_search_path)) {
        if (!(entry.is_regular_file() && entry.path().filename() == "pipelines.json")) continue;

        try {
            load_pipeline_file(entry.path().c_str());
        } catch (...) {
        }
    }
}

void DebugPipelineLoader::load_pipeline_file(const char* filename) {
    PipelineFile pipeline_file;

    auto is = std::ifstream(filename);
    cereal::JSONInputArchive archive(is);
    archive(CEREAL_NVP(pipeline_file));

    for (auto& pipeline : pipeline_file.pipelines) {
        m_pipelines_descriptions[pipeline.name] = std::make_unique<PipelineDescription>(std::move(pipeline));
    }
}

static VkPipelineBindPoint determine_pipeline_type(std::span<const CompiledShader> compiled_shaders) {
    for (const auto& shader : compiled_shaders) {
        if (shader.stage == VK_SHADER_STAGE_COMPUTE_BIT) return VK_PIPELINE_BIND_POINT_COMPUTE;
    }

    return VK_PIPELINE_BIND_POINT_GRAPHICS;
}

std::unique_ptr<IPipeline> DebugPipelineLoader::load_pipeline(PipelineDescription* description) {
    auto compiled_shaders = compile_shaders(description);

    auto pipeline_type = determine_pipeline_type(compiled_shaders);

    if (pipeline_type == VK_PIPELINE_BIND_POINT_COMPUTE) {
        vke::CPipelineBuilder builder;

        for (auto& shader : compiled_shaders) {
            builder.add_shader_stage(shader.spv, shader.stage);
        }

        m_globals_provider->set_globals(builder, description);

        return builder.build();
    } else if (pipeline_type == VK_PIPELINE_BIND_POINT_GRAPHICS) {
        vke::GPipelineBuilder builder;

        for (auto& shader : compiled_shaders) {
            builder.add_shader_stage(shader.spv, shader.stage);
        }

        m_globals_provider->set_globals(builder, description);

        builder.set_depth_testing(description->depth_test);
        builder.set_topology(description->topology_mode);
        builder.set_rasterization(description->polygon_mode, description->cull_mode);

        return builder.build();
    } else {
        THROW_ERROR("unsupported pipeline type %d", pipeline_type);
    }
}

} // namespace vke