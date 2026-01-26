#include "initialization_util.hpp"

#include "../vkutil.hpp"
#include "../vulkan_context.hpp"
#include "../window/surface.hpp"
#include "../window/window.hpp"

namespace vke {

constexpr uint32_t MAX_PHYSICAL_DEVICE = 32;

void list_physical_devices(VkInstance instance) {
    uint32_t pd_count = MAX_PHYSICAL_DEVICE;
    VkPhysicalDevice devices[MAX_PHYSICAL_DEVICE];
    vkEnumeratePhysicalDevices(instance, &pd_count, devices);

    for (int i = 0; i < pd_count; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);
        printf("found device %s\n", properties.deviceName);
    }
}

const std::array<const char*, 1> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation",
};

bool check_validation_layer_support(vk::detail::DispatchLoaderDynamic* loader) {
    uint32_t layer_count;
    VK_CHECK(loader->vkEnumerateInstanceLayerProperties(&layer_count, nullptr));

    std::vector<VkLayerProperties> availableLayers(layer_count);
    VK_CHECK(loader->vkEnumerateInstanceLayerProperties(&layer_count, availableLayers.data()));

    for (const char* layerName : VALIDATION_LAYERS) {
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                return true;
            }
        }
    }

    return false;
}

VkInstance create_instance(const ContextConfig& config, vk::detail::DispatchLoaderDynamic* loader) {
    std::vector<const char*> layer_names;
    std::vector<const char*> extension_names = config.instance_extensions;

    bool validation_enabled = config.enable_validation_layers ? check_validation_layer_support(loader) : false;

    if (validation_enabled) {
        extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        layer_names.insert(layer_names.end(), VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
    }

    auto vk_version = VK_MAKE_VERSION(config.vk_version_major, config.vk_version_minor, config.vk_version_patch);

    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = config.app_name,
        .applicationVersion = 0,
        .pEngineName        = "vke",
        .apiVersion         = vk_version,
    };

    VkInstanceCreateInfo instance_ci = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = static_cast<u32>(layer_names.size()),
        .ppEnabledLayerNames     = layer_names.data(),
        .enabledExtensionCount   = static_cast<u32>(extension_names.size()),
        .ppEnabledExtensionNames = extension_names.data(),
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_ci;
    if (validation_enabled) {
        debug_ci = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = config.validation_callback,
            .pUserData       = config.validation_callback_user_data,
        };

        instance_ci.pNext = &debug_ci;
    }

    VkInstance instance;
    VK_CHECK(loader->vkCreateInstance(&instance_ci, nullptr, &instance));
    return instance;
}

bool check_swapchain_support(const ContextConfig& config, VkInstance instance, vk::detail::DispatchLoaderDynamic* loader, VkPhysicalDevice pdevice) {
    return true;
}

double score_physical_device(const ContextConfig& config, VkInstance instance, vk::detail::DispatchLoaderDynamic* loader, VkPhysicalDevice pdevice) {
    VkPhysicalDeviceProperties properties;
    loader->vkGetPhysicalDeviceProperties(pdevice, &properties);

    double score = 0.0;

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 50'000;
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 1'000;

    score += properties.limits.maxImageDimension2D;

    if (config.window_enabled) {
        if (!check_swapchain_support(config, instance, loader, pdevice)) {
            printf("swapchain unavailable for device %s\n", properties.deviceName);
            score = -10'000;
        }
    }

    printf("device %s scored %f\n", properties.deviceName, score);

    return score;
}

VkPhysicalDevice pick_physical_device(const ContextConfig& config, VkInstance instance, vk::detail::DispatchLoaderDynamic* loader) {
    VkPhysicalDevice devices[MAX_PHYSICAL_DEVICE];
    uint32_t device_count = MAX_PHYSICAL_DEVICE;
    VK_CHECK(loader->vkEnumeratePhysicalDevices(instance, &device_count, devices));

    double max_score    = -1e10;
    int max_score_index = -1;

    for (int i = 0; i < device_count; i++) {
        double score = score_physical_device(config, instance, loader, devices[i]);

        if (score > max_score) {
            max_score       = score;
            max_score_index = i;
        }
    }

    if (max_score_index == -1) {
        printf("failed to pick physical device\n");
        return nullptr;
    }

    return devices[max_score_index];
}

std::vector<VkQueueFamilyProperties> get_queue_families(vk::detail::DispatchLoaderDynamic* loader, VkPhysicalDevice physical_device) {
    uint32_t queue_family_count = 0;
    loader->vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> families(queue_family_count);
    loader->vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, families.data());

    return families;
}

uint32_t find_queue_family(vk::detail::DispatchLoaderDynamic* loader, VkPhysicalDevice physical_device, VkQueueFlagBits queue_flags) {
    auto families = get_queue_families(loader, physical_device);

    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueCount > 0 &&
            (families[i].queueFlags & queue_flags)) {
            return i;
        }
    }

    throw std::runtime_error("No graphics queue family found");
}

bool does_family_support_present(vk::detail::DispatchLoaderDynamic* loader, VkPhysicalDevice physical_device, uint32_t family, VkSurfaceKHR surface) {
    VkBool32 does_graphics_family_support_present = VK_FALSE;
    VK_CHECK(loader->vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family,
        surface, &does_graphics_family_support_present));

    return does_graphics_family_support_present == VK_TRUE;
}

std::optional<uint32_t> find_present_queue(vk::detail::DispatchLoaderDynamic* loader, VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    auto families = get_queue_families(loader, physical_device);

    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueCount > 0 && does_family_support_present(loader, physical_device, i, surface)) {
            return i;
        }
    }

    return std::nullopt;
}

DeviceResults create_device(const ContextConfig& config, VkInstance instance, vk::detail::DispatchLoaderDynamic* loader, VkPhysicalDevice physical_device) {
    uint32_t graphics_family = -1, present_family = -1;

    graphics_family = find_queue_family(loader, physical_device, VK_QUEUE_GRAPHICS_BIT);

    if (config.window_enabled) {
        assert(config.window != nullptr);

        auto surface = config.window->surface()->get_surface();
        if (does_family_support_present(loader, physical_device, graphics_family, surface)) {
            present_family = graphics_family;
        } else {
            present_family = find_present_queue(loader, physical_device, surface).value();
        }
    } else {
        present_family = -1;
    }

    float queue_priority = 1.f;

    std::vector<VkDeviceQueueCreateInfo> queue_ci = {
        VkDeviceQueueCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphics_family,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        },
    };

    if (config.window_enabled && present_family != graphics_family) {
        queue_ci.push_back({
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = present_family,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        });
    }

    auto features1_3  = config.features1_3;
    features1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    auto features1_2  = config.features1_2;
    features1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    auto features1_1  = config.features1_1;
    features1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    VkPhysicalDeviceFeatures2 features1_0 = {
        .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .features = config.features1_0,
    };

    VkDeviceCreateInfo device_ci = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = static_cast<uint32_t>(queue_ci.size()),
        .pQueueCreateInfos       = queue_ci.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(config.device_extensions.size()),
        .ppEnabledExtensionNames = config.device_extensions.data(),
    };

    device_ci.pNext   = &features1_0;
    features1_0.pNext = &features1_1;
    features1_1.pNext = &features1_2;
    features1_2.pNext = &features1_3;
    features1_3.pNext = nullptr;

    VkDevice device = nullptr;
    VK_CHECK(loader->vkCreateDevice(physical_device, &device_ci, nullptr, &device));

    VkQueue graphics_queue, present_queue;
    loader->vkGetDeviceQueue(device, graphics_family, 0, &graphics_queue);

    if (graphics_family != present_family) {
        loader->vkGetDeviceQueue(device, present_family, 0, &present_queue);
    } else {
        present_queue = graphics_queue;
    }

    return DeviceResults{
        .device          = device,
        .graphics_family = graphics_family,
        .present_family  = present_family,
        .graphics_queue  = graphics_queue,
        .present_queue   = present_queue,
    };
}

} // namespace vke