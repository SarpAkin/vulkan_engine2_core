#pragma once

#include <vke/fwd.hpp>
#include "../vk_resource.hpp"

namespace vke {

class IPipeline : public Resource {
public:
    Resource* as_resource(){return dynamic_cast<Resource*>(this);}

    virtual VkPipeline handle()=0;
    virtual VkPipelineBindPoint bind_point()=0; 
protected:
    virtual void bind(CommandBuffer& cmd);

    friend CommandBuffer;
};

}