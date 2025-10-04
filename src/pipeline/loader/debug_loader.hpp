#pragma once

#include <vke/fwd.hpp>

#include <unordered_map>

#include "ipipeline_loader.hpp"

namespace vke {

class PipelineDescription;
class PipelineGlobalsProvider;

class DescriptorSetLayoutDescription;

class DebugPipelineLoader final : public IPipelineLoader {
public:
    DebugPipelineLoader(const DebugLoaderArguments& args);
    ~DebugPipelineLoader();

    std::unique_ptr<IPipeline> load(const char* pipeline_name) override;
    void set_pipeline_globals_provider(std::shared_ptr<PipelineGlobalsProvider> globals_provider) override;
    PipelineGlobalsProvider* get_pipeline_globals_provider() override { return m_globals_provider.get(); }

    const PipelineDescription* get_pipeline_description(const char* pipeline_name) override;
    std::vector<std::shared_ptr<const PipelineFile>> get_pipeline_files() override {return m_pipeline_files;}

private:
    void load_descriptions();
    void load_pipeline_file(const char* file_name);
    void process_description(PipelineDescription* description);
    void load_descriptor_set_layouts(const PipelineFile* pipeline_file);
    void load_descriptor_set_layout(const DescriptorSetLayoutDescription* desc);

    std::unique_ptr<IPipeline> load_pipeline(const PipelineDescription* description);
    std::string resolve_shader_lib_path(const std::string& path);

private:
    std::unordered_map<std::string, std::shared_ptr<const PipelineDescription>> m_pipelines_descriptions;

    std::vector<std::string> m_pipeline_search_paths;
    std::vector<fs::path> m_shader_lib_paths;
    std::vector<std::shared_ptr<const vke::PipelineFile>> m_pipeline_files;

    std::shared_ptr<PipelineGlobalsProvider> m_globals_provider;
};

} // namespace vke