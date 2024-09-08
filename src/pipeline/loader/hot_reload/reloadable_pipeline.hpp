#pragma once

#include "../../ipipeline.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace vke {

class ReloadableLoader;

class ReloadablePipeline : public IPipeline {
public:
    ReloadablePipeline(ReloadableLoader* loader);
    ~ReloadablePipeline();

    // can be called from any thread
    void set_pipeline(std::unique_ptr<vke::IPipeline> pipeline) {
        std::lock_guard<std::mutex> lock(m_lock);
        m_pending_pipeline = std::move(pipeline);
        m_updated          = true;
    }

    ReloadablePipeline(const ReloadablePipeline& other) = delete;

    VkPipeline handle() override { return m_current_pipeline->handle(); }
    VkPipelineBindPoint bind_point() override { return m_current_pipeline->bind_point(); }
    VkPipelineLayout layout() override { return m_current_pipeline->layout(); }
    VkShaderStageFlagBits push_stages() override { return m_current_pipeline->push_stages(); }
    VkDescriptorSetLayout set_layout(u32 index) override { return m_current_pipeline->set_layout(index); }

    void bind(vke::CommandBuffer& cmd) override;
    void reload();

public:
    std::vector<std::string> shader_files;

private:
private:
    vke::RCResource<vke::IPipeline> m_current_pipeline;
    std::unique_ptr<vke::IPipeline> m_pending_pipeline;

    ReloadableLoader* m_loader = nullptr;

    std::mutex m_lock;
    std::atomic<bool> m_updated = false;
};

} // namespace vke