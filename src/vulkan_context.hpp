#pragma once

#include <vulkan/vulkan.h>

#include "common.hpp"

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace vke {

class VulkanContext {
public:
    static void create_vulkan_context(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device);
    static void cleanup_conext();

    static VulkanContext* get_context() { return s_context; }

    VkInstance get_instance() { return m_instance; }
    VkDevice get_device() { return m_device; }
    VkPhysicalDevice get_physical_device() { return m_physical_device; }

    VmaAllocator gpu_allocator() { return m_allocator; }

    ~VulkanContext();

    VkQueue get_graphics_queue() { return m_graphics_queue; }
    u32 get_graphics_queue_family() { return m_graphics_queue_family; }

private:
    void init_vma_allocator();

    void init_queues();

    VulkanContext(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device);

private:
    static VulkanContext* s_context;

    bool device_owned = false;

    VkInstance m_instance              = nullptr;
    VkPhysicalDevice m_physical_device = nullptr;
    VkDevice m_device                  = nullptr;
    VmaAllocator m_allocator           = nullptr;

    // queues
    VkQueue m_graphics_queue;

    int m_graphics_queue_family;
};

} // namespace vke
