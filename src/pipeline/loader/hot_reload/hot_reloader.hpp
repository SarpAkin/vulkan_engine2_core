#pragma once

#include <vke/fwd.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <filesystem>

namespace fs = std::filesystem;

#include "../debug_loader.hpp"
#include "../ipipeline_loader.hpp"

namespace vke {

class ReloadablePipeline;

class ReloadableLoader : public IPipelineLoader {
public:
    ReloadableLoader(const DebugLoaderArguments& args);
    ~ReloadableLoader();

    std::unique_ptr<IPipeline> load(const char* pipeline_name) override;
    PipelineGlobalsProvider* get_pipeline_globals_provider() override { return m_pipeline_loader->get_pipeline_globals_provider(); }
    void set_pipeline_globals_provider(std::shared_ptr<PipelineGlobalsProvider> globals_provider) override;
    const PipelineDescription* get_pipeline_description(const char* pipeline_name) override { return m_pipeline_loader->get_pipeline_description(pipeline_name); }

    void remove_watched_pipeline(ReloadablePipeline* pipeline);

    // stops watching
    void stop();

private:
    struct PipelineReloadDescription;

    void worker_function();
    void reload_pipeline(ReloadablePipeline* pipeline, const PipelineReloadDescription* description);
    void find_pipelines_to_reload(std::unordered_map<ReloadablePipeline*, PipelineReloadDescription*>& pipelines_to_reload);

private:
    struct PipelineReloadDescription {
        std::string pipeline_name;
    };

    struct FileWatchInfo {
        std::vector<ReloadablePipeline*> pipelines;
        fs::file_time_type last_modification_time;
    };

    std::unique_ptr<DebugPipelineLoader> m_pipeline_loader;
    std::unordered_map<std::string, FileWatchInfo> m_watched_files;
    std::unordered_map<ReloadablePipeline*, PipelineReloadDescription> m_reload_descriptions;

    std::mutex m_lock;
    std::atomic<bool> m_running = false;
    std::thread m_thread;
};

} // namespace vke