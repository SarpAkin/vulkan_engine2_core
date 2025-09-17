#pragma once

#include <vke/fwd.hpp>

#include <unordered_map>

#include "ipipeline_loader.hpp"

namespace vke {

class PipelineDescription;
class PipelineGlobalsProvider;

class DebugPipelineLoader final : public IPipelineLoader {
public:
    DebugPipelineLoader(const DebugLoaderArguments& args);
    ~DebugPipelineLoader();

    std::unique_ptr<IPipeline> load(const char* pipeline_name) override;
    void set_pipeline_globals_provider(std::shared_ptr<PipelineGlobalsProvider> globals_provider) override { m_globals_provider = std::move(globals_provider); }
    PipelineGlobalsProvider* get_pipeline_globals_provider() override { return m_globals_provider.get(); }

    const PipelineDescription* get_pipeline_description(const char* pipeline_name);

private:
    void load_descriptions();
    void load_pipeline_file(const char* file_name);
    void process_description(PipelineDescription* description);

    std::unique_ptr<IPipeline> load_pipeline(const PipelineDescription* description);
    std::string resolve_shader_lib_path(const std::string& path);

private:
    std::unordered_map<std::string, std::unique_ptr<PipelineDescription>> m_pipelines_descriptions;

    std::vector<std::string> m_pipeline_search_paths;
    std::vector<fs::path> m_shader_lib_paths;

    std::shared_ptr<PipelineGlobalsProvider> m_globals_provider;
};

} // namespace vke