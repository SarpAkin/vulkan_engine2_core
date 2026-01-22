#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

namespace vke {

class ContextConfig;

struct DeviceResults {
    VkDevice device;
    uint32_t graphics_family,present_family;
    VkQueue graphics_queue,present_queue;
};

void list_physical_devices(VkInstance instance);
VkInstance create_instance(const ContextConfig& config, vk::detail::DispatchLoaderDynamic* loader);
VkPhysicalDevice pick_physical_device(const ContextConfig& config, VkInstance instance, vk::detail::DispatchLoaderDynamic* loader);
DeviceResults create_device(const ContextConfig& config, VkInstance instance, vk::detail::DispatchLoaderDynamic* loader, VkPhysicalDevice physical_device);

} // namespace vke
