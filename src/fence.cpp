#include "fence.hpp"

namespace vke{

Fence::Fence(bool signaled){
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    if(signaled){
        info.flags |= VK_FENCE_CREATE_SIGNALED_BIT;
    }

    vkCreateFence(device(), &info, nullptr, &m_fence);
}

Fence::~Fence(){
    vkDestroyFence(device(), m_fence, nullptr);
};


} // namespace vke