#include "debug_loader.hpp"

#include "../../builders/pipeline_builder.hpp"

#include "pipeline_file.hpp"
#include "shader_compiler/shader_compiler.hpp"

#include "pipeline_file.hpp"
#include "pipeline_globals_provider.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>

#include <vke/util.hpp>

namespace vke {

DebugPipelineLoader::~DebugPipelineLoader() {}

DebugPipelineLoader::DebugPipelineLoader(const std::vector<std::string>& pipeline_search_path) {
    m_pipeline_search_paths = pipeline_search_path;

    load_descriptions();
}

std::unique_ptr<IPipeline> DebugPipelineLoader::load(const char* pipeline_name) {
    auto it = m_pipelines_descriptions.find(pipeline_name);
    if (it == m_pipelines_descriptions.end()) {
        THROW_ERROR("pipeline %s not found", pipeline_name);
    }

    return load_pipeline(it->second.get());
}

void DebugPipelineLoader::load_descriptions() {

    for (const auto& root : m_pipeline_search_paths) {
        LOG_INFO("loading pipeline descriptions from %s", root.c_str());
        
        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            if (!(entry.is_regular_file() && entry.path().filename() == "pipelines.json")) continue;

            try {
                load_pipeline_file(entry.path().c_str());
            } catch (std::exception& e) {
                LOG_ERROR("failed to load pipeline %s: %s", entry.path().c_str(), e.what());
            }
        }
    }
}

void DebugPipelineLoader::load_pipeline_file(const char* filename) {
    PipelineFile pipeline_file;

    auto is = std::ifstream(filename);
    pipeline_file.load(json::parse(is));

    for (auto& pipeline : pipeline_file.pipelines) {
        auto name          = pipeline.name; // pipeline is moved out so name becomes empty after the move
        pipeline.file_path = filename;

        m_pipelines_descriptions[name] = std::make_unique<PipelineDescription>(std::move(pipeline));
    }

    // LOG_INFO("loaded pipeline file %s with %ld pipelines", filename, pipeline_file.pipelines.size());
}

static VkPipelineBindPoint determine_pipeline_type(std::span<const CompiledShader> compiled_shaders) {
    for (const auto& shader : compiled_shaders) {
        if (shader.stage == VK_SHADER_STAGE_COMPUTE_BIT) return VK_PIPELINE_BIND_POINT_COMPUTE;
    }

    return VK_PIPELINE_BIND_POINT_GRAPHICS;
}

std::unique_ptr<IPipeline> DebugPipelineLoader::load_pipeline(PipelineDescription* description) {
    auto compiled_shaders = m_globals_provider->shader_compiler->compile_shaders(description);

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

        builder.set_depth_testing(description->depth_test, description->depth_write, description->depth_op);
        builder.set_topology(description->topology_mode);
        builder.set_rasterization(description->polygon_mode, description->cull_mode);
        builder.set_subpass_name(description->renderpass);

        return builder.build();
    } else {
        THROW_ERROR("unsupported pipeline type %d", pipeline_type);
    }
}

const PipelineDescription* DebugPipelineLoader::get_pipeline_description(const char* pipeline_name) {
    auto it = m_pipelines_descriptions.find(pipeline_name);
    if (it == m_pipelines_descriptions.end()) {
        THROW_ERROR("pipeline %s not found", pipeline_name);
    }
    return it->second.get();
}
} // namespace vke