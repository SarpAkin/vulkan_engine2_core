#include "reloadable_pipeline.hpp"

#include "hot_reloader.hpp"

namespace vke{

ReloadablePipeline::ReloadablePipeline(ReloadableLoader* loader) : m_loader(loader) {
}


ReloadablePipeline::~ReloadablePipeline() {
    m_loader->remove_watched_pipeline(this);
}

void ReloadablePipeline::reload() {
    if (!m_updated) return;

    std::lock_guard<std::mutex> lock(m_lock);
    m_current_pipeline = std::move(m_pending_pipeline);
    this->set_external(m_current_pipeline->as_resource());

    m_updated = false;
}

void ReloadablePipeline::bind(vke::CommandBuffer& cmd) {
    reload();
    m_current_pipeline->bind(cmd);
}
} // namespace vke