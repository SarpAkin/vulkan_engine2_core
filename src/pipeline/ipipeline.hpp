#pragma once

#include "../vk_resource.hpp"
#include <vke/fwd.hpp>

namespace vke {

class IPipeline : public Resource {
public:
    Resource* as_resource() { return dynamic_cast<Resource*>(this); }

    ~IPipeline() = default;
    IPipeline()  = default;

    virtual VkPipeline handle()                         = 0;
    virtual VkPipelineBindPoint bind_point()            = 0;
    virtual VkPipelineLayout layout()                   = 0;
    virtual VkShaderStageFlagBits push_stages()         = 0;
    virtual VkDescriptorSetLayout set_layout(u32 index) = 0;
    virtual std::string_view subpass_name()             = 0;

    protected:
    virtual void bind(CommandBuffer& cmd); // for internal use do not call
    friend class ReloadablePipeline;
    friend CommandBuffer;
};

} // namespace vke