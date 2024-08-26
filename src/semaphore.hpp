#pragma once

#include "vk_resource.hpp"

namespace vke {


class Semaphore : public Resource {
public:
    VkSemaphore handle() const { return m_semaphore; }

    Semaphore();
    ~Semaphore();

private:
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
};


}