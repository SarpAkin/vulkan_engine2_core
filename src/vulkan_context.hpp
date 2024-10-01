#pragma once

#include <memory>
#include <vulkan/vulkan.h>

#include "common.hpp"

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace vke {

struct DeviceInfo{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceFeatures features;
};

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

    DeviceInfo* get_device_info()const { return m_device_info.get(); }

    VkFence get_thread_local_fence();

private:
    void init_vma_allocator();

    void init_queues();

    void query_device_info();

    VulkanContext(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device);

private:
    static VulkanContext* s_context;

    bool device_owned = false;

    VkInstance m_instance              = nullptr;
    VkPhysicalDevice m_physical_device = nullptr;
    VkDevice m_device                  = nullptr;
    VmaAllocator m_allocator           = nullptr;

    std::unique_ptr<DeviceInfo> m_device_info;

    // queues
    VkQueue m_graphics_queue;

    int m_graphics_queue_family;
};

} // namespace vke
