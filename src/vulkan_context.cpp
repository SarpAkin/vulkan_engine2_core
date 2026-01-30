#include "vulkan_context.hpp"

#include <cstdio>
#include <stdexcept>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "builders/descriptor_set_layout_builder.hpp"
#include "commandbuffer.hpp"
#include "fence.hpp"
#include "util/util.hpp"
#include "vkutil.hpp"

#include "init/initialization_util.hpp"
#include "window/window.hpp"

namespace vke {

struct VulkanContext::Handles {
    vk::Instance instance              = nullptr;
    vk::Device device                  = nullptr;
    vk::PhysicalDevice physical_device = nullptr;
    vk::detail::DispatchLoaderDynamic dispatch_table;
};

void load_dispatch_table(VulkanContext::Handles& handles, bool device_specific) {
    vk::detail::DynamicLoader dl;

    if (device_specific) {
        handles.dispatch_table.init(handles.instance, handles.device, dl);
    } else {
        handles.dispatch_table.init(dl);
    }
}

VulkanContext* VulkanContext::s_context;

void VulkanContext::create_vulkan_context(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device) {
    s_context = new VulkanContext(instance, pdevice, device);
}

VulkanContext::VulkanContext(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device) {
    m_instance        = instance;
    m_device          = device;
    m_physical_device = pdevice;

    query_device_info();

    vk::CommandBuffer cmd;
    constexpr uint32_t size = sizeof(cmd);

    ContextConfig config{
        .device_memory_addres = true,
    };

    init_vma_allocator(config);
    init_queues();
}

void VulkanContext::init(const ContextConfig& config) {
    ContextConfig config2 = config;

    s_context = new VulkanContext(config);
}

VulkanContext::VulkanContext(const ContextConfig& config) {
    device_owned = true;

    init_context(config);

    query_device_info();

    init_vma_allocator(config);
    init_queues();
}

VulkanContext::~VulkanContext() {
    DescriptorSetLayoutBuilder::cleanup_layouts();

    vmaDestroyAllocator(m_allocator);

    if (device_owned) {
        vkDestroyDevice(m_device, nullptr);
        vkDestroyInstance(m_instance, nullptr);
    }
}

const char* to_string_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT s) {
    switch (s) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERBOSE";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "ERROR";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARNING";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "INFO";
    default: return "UNKNOWN";
    }
}
const char* to_string_message_type(VkDebugUtilsMessageTypeFlagsEXT s) {
    if (s == 7) return "General | Validation | Performance";
    if (s == 6) return "Validation | Performance";
    if (s == 5) return "General | Performance";
    if (s == 4 /*VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/) return "Performance";
    if (s == 3) return "General | Validation";
    if (s == 2 /*VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT*/) return "Validation";
    if (s == 1 /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT*/) return "General";
    return "Unknown";
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*) {
    auto ms = to_string_message_severity(messageSeverity);
    auto mt = to_string_message_type(messageType);
    printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

    return VK_FALSE; // Applications must return false here
}

void validate_config(ContextConfig& config) {
    if (config.device_memory_addres) {
        config.features1_2.bufferDeviceAddress = true;
    }
}

void VulkanContext::init_context(const ContextConfig& _config) {
    ContextConfig config = _config;
#ifndef NDEBUG
    config.enable_validation_layers = true;
#endif
    if (config.enable_validation_layers && config.validation_callback == nullptr) {
        config.validation_callback = debug_callback;
    }

    if (config.window) config.window_enabled = true;

    if (config.window_enabled) {
        config.device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        config.instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        auto window_extensions = config.window->get_instance_extensions();
        config.instance_extensions.insert(config.instance_extensions.end(), window_extensions.begin(), window_extensions.end());
    }

    validate_config(config);

    m_handles = std::make_unique<Handles>();
    load_dispatch_table(*m_handles, false);

    m_instance = create_instance(config, &m_handles->dispatch_table);

    // m_handles->instance = m_instance;
    // load_dispatch_table(*m_handles, true);

    if (config.window) {
        config.window->init_surface(this);
    }

    m_physical_device = pick_physical_device(config, m_instance, &m_handles->dispatch_table);

    auto device_result = create_device(config, m_instance, &m_handles->dispatch_table, m_physical_device);

    m_device = device_result.device;

    m_handles->instance        = m_instance;
    m_handles->device          = m_device;
    m_handles->physical_device = m_physical_device;

    load_dispatch_table(*m_handles, true);
}

void VulkanContext::init_vma_allocator(const ContextConfig& config) {
    VmaVulkanFunctions vulkan_functions = {
        .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = &vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo create_info{
        .physicalDevice   = m_physical_device,
        .device           = m_device,
        .pVulkanFunctions = &vulkan_functions,
        .instance         = m_instance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };

    if (config.device_memory_addres) {
        create_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }

    vmaCreateAllocator(&create_info, &m_allocator);

    printf("created VMA allocator from c++\n");
}

void VulkanContext::init_queues() {
    // Find a suitable queue family
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, queueFamilies.data());

    int graphicsFamily = -1;
    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            break;
        }
    }

    if (graphicsFamily == -1) {
        throw std::runtime_error("failed to find a graphics queue family!");
    }

    // Initialize the graphics queue
    vkGetDeviceQueue(m_device, graphicsFamily, 0, &m_graphics_queue);
    m_graphics_queue_family = graphicsFamily;
}

void VulkanContext::cleanup_conext() { delete s_context; }

void VulkanContext::query_device_info() {
    m_device_info = std::make_unique<DeviceInfo>();

    dt().vkGetPhysicalDeviceProperties(m_physical_device, &m_device_info->properties);
    dt().vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_device_info->memory_properties);

    VkPhysicalDeviceFeatures2 features2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &m_device_info->features1_1,
    };

    m_device_info->features1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_device_info->features1_1.pNext = &m_device_info->features1_2;
    m_device_info->features1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_device_info->features1_2.pNext = &m_device_info->features1_3;
    m_device_info->features1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    dt().vkGetPhysicalDeviceFeatures2(m_physical_device, &features2);
    m_device_info->features = features2.features;
}

thread_local std::unique_ptr<vke::Fence> thread_local_fence = nullptr;

VkFence VulkanContext::get_thread_local_fence() {
    if (!thread_local_fence) {
        thread_local_fence = std::make_unique<vke::Fence>();
    }

    return thread_local_fence->handle();
}

void VulkanContext::immediate_submit(std::function<void(vke::CommandBuffer& cmd)> function) {
    vke::CommandBuffer cmd;

    cmd.begin();
    function(cmd);
    cmd.end();

    VkSubmitInfo info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd.handle(),
    };

    CommandBuffer* cmds[] = {&cmd};

    VkFence fence = get_thread_local_fence();

    VK_CHECK(vkQueueSubmit(get_graphics_queue(), 1, &info, fence));

    VK_CHECK(vkWaitForFences(get_device(), 1, &fence, VK_TRUE, 1E10));
    VK_CHECK(vkResetFences(get_device(), 1, &fence));
}

vk::Device VulkanContext::get_cpp_device() const { return m_device; }
vk::Instance VulkanContext::get_cpp_instance() const { return m_instance; }
vk::PhysicalDevice VulkanContext::get_cpp_physical_device() const { return m_physical_device; }
const vk::detail::DispatchLoaderDynamic& VulkanContext::get_dispatch_table() const { return m_handles->dispatch_table; }
} // namespace vke
