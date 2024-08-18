#include "semaphore.hpp"

#include "vulkan_context.hpp"

namespace vke {

Semaphore::Semaphore() {
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        
    };

    vkCreateSemaphore(device(), &info, nullptr, &m_semaphore);
};

Semaphore::~Semaphore(){
    vkDestroySemaphore(device(), m_semaphore, nullptr);
};



} // namespace vke