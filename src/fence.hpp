#pragma once

#include "vk_resource.hpp"

namespace vke {


class Fence : public Resource {
public:
    VkFence handle() const { return m_fence; }

    Fence(bool signaled = false);
    ~Fence();

private:
    VkFence m_fence = VK_NULL_HANDLE;
};


}