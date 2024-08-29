#pragma once

#include <vke/fwd.hpp>
#include "../vk_resource.hpp"

namespace vke {

class IPipeline : public Resource {
public:
    Resource* as_resource(){return dynamic_cast<Resource*>(this);}

    ~IPipeline()=default;
    IPipeline()=default;

    virtual VkPipeline handle()=0;
    virtual VkPipelineBindPoint bind_point()=0; 
    virtual VkPipelineLayout layout()=0;
    virtual VkShaderStageFlagBits push_stages()=0;
    virtual VkDescriptorSetLayout set_layout(u32 index) = 0;
protected:
    virtual void bind(CommandBuffer& cmd);

    friend CommandBuffer;
};

}