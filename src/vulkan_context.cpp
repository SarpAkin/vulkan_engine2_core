#include "vulkan_context.hpp"

#include <cstdio>
#include <stdexcept>
#include <vector>
#include <vk_mem_alloc.h>

#include "builders/descriptor_set_layout_builder.hpp"

namespace vke {

VulkanContext* VulkanContext::s_context;

void VulkanContext::create_vulkan_context(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device) {
    s_context = new VulkanContext(instance, pdevice, device);
}

VulkanContext::VulkanContext(VkInstance instance, VkPhysicalDevice pdevice, VkDevice device) {
    m_instance        = instance;
    m_device          = device;
    m_physical_device = pdevice;

    init_vma_allocator();
    init_queues();
}

VulkanContext::~VulkanContext() {
    DescriptorSetLayoutBuilder::cleanup_layouts();

    vmaDestroyAllocator(m_allocator);


    if(device_owned){
        //TODO destroy device & stuff
    }
}

void VulkanContext::init_vma_allocator() {
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
}

void VulkanContext::cleanup_conext() { delete s_context; }
} // namespace vke