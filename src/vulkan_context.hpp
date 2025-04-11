#pragma once

#include <functional>
#include <memory>
#include <vulkan/vulkan.h>

#include "common.hpp"
#include "fwd.hpp"

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace vke {

struct DeviceInfo {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VkPhysicalDeviceFeatures features            = {};
    VkPhysicalDeviceVulkan11Features features1_1 = {};
    VkPhysicalDeviceVulkan12Features features1_2 = {};
    VkPhysicalDeviceVulkan13Features features1_3 = {};
};

struct ContextConfig;

class VulkanContext {
public:
    struct Handles;

public:
    static void init(const ContextConfig& config);
    static void create_vulkan_context(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device);
    static void cleanup_conext();

    static VulkanContext* get_context() { return s_context; }

    VkInstance get_instance() { return m_instance; }
    VkDevice get_device() { return m_device; }
    VkPhysicalDevice get_physical_device() { return m_physical_device; }

    vk::Device get_cpp_device() const;
    vk::Instance get_cpp_instance() const;
    vk::PhysicalDevice get_cpp_physical_device() const;
    const vk::detail::DispatchLoaderDynamic& get_dispatch_table() const;

    VmaAllocator gpu_allocator() { return m_allocator; }

    ~VulkanContext();

    VkQueue get_graphics_queue() { return m_graphics_queue; }
    u32 get_graphics_queue_family() { return m_graphics_queue_family; }

    DeviceInfo* get_device_info() const { return m_device_info.get(); }

    VkFence get_thread_local_fence();

    void immediate_submit(std::function<void(vke::CommandBuffer& cmd)> function);

private:
    void init_context(const ContextConfig& config);
    void init_context2(const ContextConfig& config);

    void init_vma_allocator(const ContextConfig& config);
    void init_queues();
    void query_device_info();

    const vk::detail::DispatchLoaderDynamic& dt() const { return get_dispatch_table(); }

    VulkanContext(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device);
    VulkanContext(const ContextConfig& config);

private:
    static VulkanContext* s_context;

    bool device_owned = false;

    std::unique_ptr<Handles> m_handles;

    VkInstance m_instance              = nullptr;
    VkPhysicalDevice m_physical_device = nullptr;
    VkDevice m_device                  = nullptr;
    VmaAllocator m_allocator           = nullptr;

    std::unique_ptr<DeviceInfo> m_device_info;

    // queues
    VkQueue m_graphics_queue;

    int m_graphics_queue_family;
};

struct ContextConfig {
    const char* app_name      = "Default App Name";
    u32 vk_version_major      = 1;
    u32 vk_version_minor      = 3;
    u32 vk_version_patch      = 0;
    bool window               = true;
    bool device_memory_addres = false;
    // Window* window       = nullptr;
    VkPhysicalDeviceFeatures features1_0                       = {};
    VkPhysicalDeviceVulkan11Features features1_1               = {};
    VkPhysicalDeviceVulkan12Features features1_2               = {};
    VkPhysicalDeviceVulkan13Features features1_3               = {};
    VkPhysicalDeviceMeshShaderFeaturesEXT features_mesh_shader = {};
};

} // namespace vke
