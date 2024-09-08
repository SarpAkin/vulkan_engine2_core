#include "hot_reloader.hpp"

#include <unordered_set>
#include <vke/util.hpp>

#include "../debug_loader.hpp"
#include "../pipeline_globals_provider.hpp"
#include "reloadable_pipeline.hpp"
#include <filesystem>

#include "../pipeline_file.hpp"

namespace vke {

ReloadableLoader::ReloadableLoader(const char* pipeline_search_path) : IPipelineLoader() {
    m_pipeline_loader = std::make_unique<DebugPipelineLoader>(pipeline_search_path);

    m_running = true;

    m_thread = std::thread([this]() { worker_function(); });
    name_thread(m_thread, "shader hot reloader");
}

ReloadableLoader::~ReloadableLoader() {
    if (m_running) stop();
}

std::unique_ptr<IPipeline> ReloadableLoader::load(const char* pipeline_name) {
    auto* description = m_pipeline_loader->get_pipeline_description(pipeline_name);
    auto pipeline     = m_pipeline_loader->load(pipeline_name);

    auto reloadable_pipeline = std::make_unique<ReloadablePipeline>(this);
    reloadable_pipeline->set_pipeline(std::move(pipeline));
    reloadable_pipeline->reload();

    fs::path base_path = fs::path(description->file_path).parent_path();

    reloadable_pipeline->shader_files = vke::map_vec(description->shader_files, [&](const std::string& s) {
        return (base_path / s).string();
    });

    std::lock_guard<std::mutex> lock(m_lock);
    m_reload_descriptions[reloadable_pipeline.get()] = {pipeline_name};

    for (auto& file : reloadable_pipeline->shader_files) {
        if (!m_watched_files.contains(file)) {
            m_watched_files.insert({file, FileWatchInfo{
                                              .last_modification_time = fs::last_write_time(file),
                                          }});
        }

        m_watched_files[file].pipelines.push_back(reloadable_pipeline.get());
    }

    return reloadable_pipeline;
}

void ReloadableLoader::worker_function() {
    std::unordered_map<ReloadablePipeline*, PipelineReloadDescription*> pipelines_to_reload;
    pipelines_to_reload.reserve(16);

    while (m_running) {
        pipelines_to_reload.clear();

        find_pipelines_to_reload(pipelines_to_reload);

        for (auto& [pipeline, description] : pipelines_to_reload) {
            reload_pipeline(pipeline, description);
        }

        // sleep for .5 seconds
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void ReloadableLoader::find_pipelines_to_reload(std::unordered_map<ReloadablePipeline*, PipelineReloadDescription*>& pipelines_to_reload) {
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto& [file, info] : m_watched_files) {
        auto time = fs::last_write_time(file);

        if (time != info.last_modification_time) {
            info.last_modification_time = time;

            for (auto pipeline : info.pipelines) {
                if (pipelines_to_reload.contains(pipeline)) continue;

                pipelines_to_reload[pipeline] = &m_reload_descriptions[pipeline];
            }
        }
    }
}

void ReloadableLoader::reload_pipeline(ReloadablePipeline* reloadable_pipeline, const PipelineReloadDescription* description) {
    try {
        auto pipeline = m_pipeline_loader->load(description->pipeline_name.c_str());
        reloadable_pipeline->set_pipeline(std::move(pipeline));
    } catch (std::exception& e) {
        LOG_ERROR("failed to reload pipeline %s: %s", description->pipeline_name.c_str(), e.what());
    }
}

void ReloadableLoader::remove_watched_pipeline(ReloadablePipeline* pipeline) {
    std::lock_guard<std::mutex> lock(m_lock);

    m_reload_descriptions.erase(pipeline);

    for (auto files : pipeline->shader_files) {

        auto it = m_watched_files.find(files);

        if (it == m_watched_files.end()) continue;
        auto& info = it->second;

        info.pipelines.erase(std::remove(info.pipelines.begin(), info.pipelines.end(), pipeline), info.pipelines.end());
    }
}
void ReloadableLoader::set_pipeline_globals_provider(std::unique_ptr<class PipelineGlobalsProvider> globals_provider) {
    m_pipeline_loader->set_pipeline_globals_provider(std::move(globals_provider));
}

void ReloadableLoader::stop() {
    if (!m_running) {
        LOG_WARNING("trying to stop pipeline hot reloader that is not running");
        return;
    }

    m_running = false;

    if (m_thread.joinable()) {
        m_thread.join();
    }
}
} // namespace vke
