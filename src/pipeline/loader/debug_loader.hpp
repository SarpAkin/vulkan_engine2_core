#pragma once

#include <vke/fwd.hpp>

#include <unordered_map>

#include "ipipeline_loader.hpp"

namespace vke{

class PipelineDescription;

class DebugPipelineLoader : public IPipelineLoader
{
public:
    DebugPipelineLoader(const char* pipeline_search_path);
    ~DebugPipelineLoader();

    std::unique_ptr<IPipeline> load(const char* pipeline_name) final override;
private:
    void load_descriptions();
    void load_pipeline_file(const char* file_name);

    std::unique_ptr<IPipeline> load_pipeline(PipelineDescription* description);

private:
    std::unordered_map<std::string, std::unique_ptr<PipelineDescription>> m_pipelines_descriptions;

    std::string m_pipeline_search_path;

    std::unique_ptr<class PipelineGlobalsProvider> m_globals_provider;
};

}