#include "vk.h"
#include "logger.h"
#include "macros.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace inferno
{
namespace vk
{
// -----------------------------------------------------------------------------------------------------------------------------------

const char* kDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// -----------------------------------------------------------------------------------------------------------------------------------

const std::vector<const char*> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// -----------------------------------------------------------------------------------------------------------------------------------

const char* kDeviceTypes[] = {
    "VK_PHYSICAL_DEVICE_TYPE_OTHER",
    "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU",
    "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU",
    "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU",
    "VK_PHYSICAL_DEVICE_TYPE_CPU"
};

// -----------------------------------------------------------------------------------------------------------------------------------

const char* kVendorNames[] = {
    "Unknown",
    "AMD",
    "IMAGINATION",
    "NVIDIA",
    "ARM",
    "QUALCOMM",
    "INTEL"
};

// -----------------------------------------------------------------------------------------------------------------------------------

const char* get_vendor_name(uint32_t id)
{
    switch (id)
    {
        case 0x1002:
            return kVendorNames[1];
        case 0x1010:
            return kVendorNames[2];
        case 0x10DE:
            return kVendorNames[3];
        case 0x13B5:
            return kVendorNames[4];
        case 0x5143:
            return kVendorNames[5];
        case 0x8086:
            return kVendorNames[6];
        default:
            return kVendorNames[0];
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    INFERNO_LOG_ERROR("(Vulkan) Validation Layer: " + std::string(pCallbackData->pMessage));
    return VK_FALSE;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool QueueInfos::asynchronous_compute()
{
    return compute_queue_index != graphics_queue_index;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool QueueInfos::transfer()
{
    return transfer_queue_index != compute_queue_index && transfer_queue_index != graphics_queue_index;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Backend::Backend(GLFWwindow* window, bool enable_validation_layers)
{
    VkApplicationInfo appInfo;
    INFERNO_ZERO_MEMORY(appInfo);

    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Inferno Runtime";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "Inferno";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = required_extensions(enable_validation_layers);

    VkInstanceCreateInfo create_info;
    INFERNO_ZERO_MEMORY(create_info);

    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &appInfo;
    create_info.enabledExtensionCount   = extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info;

    if (enable_validation_layers)
    {
        INFERNO_ZERO_MEMORY(debug_create_info);
        create_info.enabledLayerCount   = static_cast<uint32_t>(kValidationLayers.size());
        create_info.ppEnabledLayerNames = kValidationLayers.data();

        debug_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;

        create_info.pNext = &debug_create_info;
    }
    else
    {
        create_info.enabledLayerCount = 0;
        create_info.pNext             = nullptr;
    }

    if (vkCreateInstance(&create_info, nullptr, &m_vk_instance) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Vulkan instance.");
        throw std::runtime_error("(Vulkan) Failed to create Vulkan instance.");
    }

    if (enable_validation_layers && create_debug_utils_messenger(m_vk_instance, &debug_create_info, nullptr, &m_vk_debug_messenger) != VK_SUCCESS)
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Vulkan debug messenger.");

    if (!create_surface(window))
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Vulkan surface.");
        throw std::runtime_error("(Vulkan) Failed to create Vulkan surface.");
    }

    if (!find_physical_device())
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to find a suitable GPU.");
        throw std::runtime_error("(Vulkan) Failed to find a suitable GPU.");
    }

	if (!create_logical_device())
    {
		INFERNO_LOG_FATAL("(Vulkan) Failed to create logical device.");
		throw std::runtime_error("(Vulkan) Failed to create logical device.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Backend::~Backend()
{
    if (m_vk_debug_messenger)
        destroy_debug_utils_messenger(m_vk_instance, m_vk_debug_messenger, nullptr);

	vkDestroySurfaceKHR(m_vk_instance, m_vk_surface, nullptr);
    vkDestroyInstance(m_vk_instance, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, &available_extensions[0]);

    int unavailable_extensions = sizeof(kDeviceExtensions) / sizeof(const char*);

    for (auto& str : kDeviceExtensions)
    {
        for (const auto& extension : available_extensions)
        {
            if (strcmp(str, extension.extensionName) == 0)
                unavailable_extensions--;
        }
    }

    return unavailable_extensions == 0;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Backend::query_swap_chain_support(VkPhysicalDevice device, SwapChainSupportDetails& details)
{
    // Get surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vk_surface, &details.capabilities);

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vk_surface, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vk_surface, &present_mode_count, &details.present_modes[0]);
    }

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vk_surface, &format_count, nullptr);

    if (format_count != 0)
    {
        details.format.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vk_surface, &format_count, &details.format[0]);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::check_validation_layer_support(std::vector<const char*> layers)
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    VkLayerProperties available_layers[32];
    vkEnumerateInstanceLayerProperties(&layer_count, &available_layers[0]);

    for (const char* layer_name : layers)
    {
        bool layer_found = false;

        for (const auto& layer_properties : available_layers)
        {
            if (std::string(layer_name) == std::string(layer_properties.layerName))
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            INFERNO_LOG_FATAL("(Vulkan) Validation Layer not available: " + std::string(layer_name));
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

std::vector<const char*> Backend::required_extensions(bool enable_validation_layers)
{
    uint32_t     glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enable_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}
// -----------------------------------------------------------------------------------------------------------------------------------

VkResult Backend::create_debug_utils_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Backend::destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::create_surface(GLFWwindow* window)
{
    return glfwCreateWindowSurface(m_vk_instance, window, nullptr, &m_vk_surface) == VK_SUCCESS;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::find_physical_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_vk_instance, &device_count, nullptr);

    if (device_count == 0)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to find GPUs with Vulkan support!");
        throw std::runtime_error("(Vulkan) Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
   
    vkEnumeratePhysicalDevices(m_vk_instance, &device_count, devices.data());
    

    // Try to find a discrete GPU...
    for (const auto& device : devices)
    {
        QueueInfos infos;

        if (is_device_suitable(device, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, infos))
        {
            m_vk_physical_device = device;
            m_selected_queues    = infos;
            return true;
        }
    }

    // ...If not, try to find an integrated GPU...
    for (const auto& device : devices)
    {
        QueueInfos infos;

        if (is_device_suitable(device, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, infos))
        {
            m_vk_physical_device = device;
            m_selected_queues    = infos;
            return true;
        }
    }

    return false;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::is_device_suitable(VkPhysicalDevice device, VkPhysicalDeviceType type, QueueInfos& infos)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    uint32_t vendorId = properties.vendorID;

    if (properties.deviceType == type)
    {
        bool                    extensions_supported = check_device_extension_support(device);
        SwapChainSupportDetails details;
        query_swap_chain_support(device, details);

        if (details.format.size() > 0 && details.present_modes.size() > 0 && extensions_supported)
        {
            INFERNO_LOG_INFO("(Vulkan) Vendor : " + std::string(get_vendor_name(properties.vendorID)));
            INFERNO_LOG_INFO("(Vulkan) Name   : " + std::string(properties.deviceName));
            INFERNO_LOG_INFO("(Vulkan) Type   : " + std::string(kDeviceTypes[properties.deviceType]));
            INFERNO_LOG_INFO("(Vulkan) Driver : " + std::to_string(properties.driverVersion));

            return find_queues(device, infos);
        }
    }

    return false;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::find_queues(VkPhysicalDevice device, QueueInfos& infos)
{
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);

    INFERNO_LOG_INFO("(Vulkan) Number of Queue families: " + std::to_string(family_count));

    VkQueueFamilyProperties families[32];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, &families[0]);

    for (uint32_t i = 0; i < family_count; i++)
    {
        VkQueueFlags bits = families[i].queueFlags;

        INFERNO_LOG_INFO("(Vulkan) Family " + std::to_string(i));
        INFERNO_LOG_INFO("(Vulkan) Supported Bits: ");
        INFERNO_LOG_INFO("(Vulkan) VK_QUEUE_GRAPHICS_BIT: " + std::to_string((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) > 0));
        INFERNO_LOG_INFO("(Vulkan) VK_QUEUE_COMPUTE_BIT: " + std::to_string((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) > 0)); 
        INFERNO_LOG_INFO("(Vulkan) VK_QUEUE_TRANSFER_BIT: " + std::to_string((families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) > 0)); 
        INFERNO_LOG_INFO("(Vulkan) Number of Queues: " + std::to_string(families[i].queueCount));

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vk_surface, &present_support);

        // Look for Presentation Queue
        if (present_support && infos.presentation_queue_index == -1)
            infos.presentation_queue_index = i;

        // Look for a graphics queue if the ideal one isn't found yet.
        if (infos.graphics_queue_quality != 3)
        {
            if (is_queue_compatible(bits, 1, 1, 1))
            {
                // Ideally, a queue that supports everything.
                infos.graphics_queue_index           = i;
                infos.graphics_queue_quality = 3;
            }
            else if (is_queue_compatible(bits, 1, -1, -1))
            {
                // If not, a queue that supports at least graphics.
                infos.graphics_queue_index   = i;
                infos.graphics_queue_quality       = 1;
            }
        }

        // Look for a compute queue if the ideal one isn't found yet.
        if (infos.compute_queue_quality != 3)
        {
            if (is_queue_compatible(bits, 0, 1, 0))
            {
                // Ideally, a queue that only supports compute (for asynchronous compute).
                infos.compute_queue_index           = i;
                infos.compute_queue_quality = 3;
            }
            else if (is_queue_compatible(bits, 0, 1, 1))
            {
                // Else, a queue that supports compute and transfer only (might allow asynchronous compute. Have to check).
                infos.compute_queue_index           = i;
                infos.compute_queue_quality = 2;
            }
            else if (is_queue_compatible(bits, -1, 1, -1) && infos.compute_queue_quality == 0)
            {
                // If not, a queue that supports at least compute
                infos.compute_queue_index           = i;
                infos.compute_queue_quality = 1;
            }
        }

        // Look for a Transfer queue if the ideal one isn't found yet.
        if (infos.transfer_queue_quality != 3)
        {
            if (is_queue_compatible(bits, 0, 0, 1))
            {
                // Ideally, a queue that only supports transfer (for DMA).
                infos.transfer_queue_index           = i;
                infos.transfer_queue_quality = 3;
            }
            else if (is_queue_compatible(bits, 0, 1, 1))
            {
                // Else, a queue that supports compute and transfer only.
                infos.transfer_queue_index           = i;
                infos.transfer_queue_quality = 2;
            }
            else if (is_queue_compatible(bits, -1, -1, 1) && infos.transfer_queue_quality == 0)
            {
                // If not, a queue that supports at least graphics
                infos.transfer_queue_index           = i;
                infos.transfer_queue_quality = 1;
            }
        }
    }

    if (infos.presentation_queue_index == -1)
    {

        INFERNO_LOG_INFO("(Vulkan) No Presentation Queue Found");
        return false;
    }

    if (infos.graphics_queue_quality == 0)
    {
        INFERNO_LOG_INFO("(Vulkan) No Graphics Queue Found");
        return false;
    }

	if (infos.compute_queue_quality == 0 || infos.transfer_queue_quality == 0)
	{
		INFERNO_LOG_INFO("(Vulkan) No Queues supporting Compute or Transfer found");
		return false;
	}

    VkDeviceQueueCreateInfo presentation_queue_info;
	INFERNO_ZERO_MEMORY(presentation_queue_info);

    presentation_queue_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    presentation_queue_info.queueFamilyIndex        = infos.presentation_queue_index;
    presentation_queue_info.queueCount              = 1;

    VkDeviceQueueCreateInfo graphics_queue_info;
    INFERNO_ZERO_MEMORY(graphics_queue_info);

    graphics_queue_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphics_queue_info.queueFamilyIndex        = infos.graphics_queue_index;
    graphics_queue_info.queueCount              = 1;
 
    VkDeviceQueueCreateInfo compute_queue_info;
    INFERNO_ZERO_MEMORY(compute_queue_info);

    compute_queue_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    compute_queue_info.queueFamilyIndex        = infos.compute_queue_index;
    compute_queue_info.queueCount              = 1;

    VkDeviceQueueCreateInfo transfer_queue_info;
    INFERNO_ZERO_MEMORY(transfer_queue_info);

    transfer_queue_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    transfer_queue_info.queueFamilyIndex        = infos.transfer_queue_index;
    transfer_queue_info.queueCount              = 1;

    infos.infos[infos.queue_count++] = presentation_queue_info;

    if (infos.graphics_queue_index != infos.presentation_queue_index)
        infos.infos[infos.queue_count++] = graphics_queue_info;

    if (infos.compute_queue_index != infos.presentation_queue_index && infos.compute_queue_index != infos.graphics_queue_index)
        infos.infos[infos.queue_count++] = compute_queue_info;

    if (infos.transfer_queue_index != infos.presentation_queue_index && infos.transfer_queue_index != infos.graphics_queue_index && infos.transfer_queue_index != infos.compute_queue_index)
        infos.infos[infos.queue_count++] = transfer_queue_info;

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::is_queue_compatible(VkQueueFlags current_queue_flags, int32_t graphics, int32_t compute, int32_t transfer)
{
    if (graphics == 1)
    {
        // If you need graphics, and queue doesn't have it...
        if (!(current_queue_flags & VK_QUEUE_GRAPHICS_BIT))
            return false;
    }
    else if (graphics == 0)
    {
        // If you don't need graphics, but queue has it...
        if (current_queue_flags & VK_QUEUE_GRAPHICS_BIT)
            return false;
    }

    if (compute == 1)
    {
        // If you need compute, and queue doesn't have it...
        if (!(current_queue_flags & VK_QUEUE_COMPUTE_BIT))
            return false;
    }
    else if (compute == 0)
    {
        // If you don't need compute, but queue has it...
        if (current_queue_flags & VK_QUEUE_COMPUTE_BIT)
            return false;
    }

    if (transfer == 1)
    {
        // If you need transfer, and queue doesn't have it...
        if (!(current_queue_flags & VK_QUEUE_TRANSFER_BIT))
            return false;
    }
    else if (transfer == 0)
    {
        // If you don't need transfer, but queue has it...
        if (current_queue_flags & VK_QUEUE_TRANSFER_BIT)
            return false;
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::create_logical_device()
{
    VkPhysicalDeviceFeatures features;
    INFERNO_ZERO_MEMORY(features);

    VkDeviceCreateInfo device_info;
    INFERNO_ZERO_MEMORY(device_info);

    device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos       = &m_selected_queues.infos[0];
    device_info.queueCreateInfoCount    = static_cast<uint32_t>(m_selected_queues.queue_count);
    device_info.pEnabledFeatures        = &features;
    device_info.enabledExtensionCount   = sizeof(kDeviceExtensions) / sizeof(const char*);
    device_info.ppEnabledExtensionNames = &kDeviceExtensions[0];

	if (m_vk_debug_messenger)
    {
        device_info.enabledLayerCount   = kValidationLayers.size();
        device_info.ppEnabledLayerNames = &kValidationLayers[0];
    }
    else
        device_info.enabledLayerCount = 0;

	float priority = 1.0f;

	for (int i = 0; i < m_selected_queues.queue_count; i++)
		m_selected_queues.infos[i].pQueuePriorities = &priority;

    if (vkCreateDevice(m_vk_physical_device, &device_info, nullptr, &m_vk_device) != VK_SUCCESS)
        return false;

    // Get presentation queue
    vkGetDeviceQueue(m_vk_device, m_selected_queues.presentation_queue_index, 0, &m_vk_presentation_queue);

    // Get graphics queue
    if (m_selected_queues.graphics_queue_index == m_selected_queues.presentation_queue_index)
        m_vk_graphics_queue = m_vk_presentation_queue;
    else
        vkGetDeviceQueue(m_vk_device, m_selected_queues.graphics_queue_index, 0, &m_vk_graphics_queue);

    // Get compute queue
    if (m_selected_queues.compute_queue_index == m_selected_queues.presentation_queue_index)
        m_vk_compute_queue = m_vk_presentation_queue;
    else if (m_selected_queues.compute_queue_index == m_selected_queues.graphics_queue_index)
        m_vk_compute_queue = m_vk_graphics_queue;
    else
        vkGetDeviceQueue(m_vk_device, m_selected_queues.compute_queue_index, 0, &m_vk_compute_queue);

    // Get transfer queue
    if (m_selected_queues.transfer_queue_index == m_selected_queues.presentation_queue_index)
        m_vk_transfer_queue = m_vk_presentation_queue;
    else if (m_selected_queues.transfer_queue_index == m_selected_queues.graphics_queue_index)
        m_vk_transfer_queue = m_vk_graphics_queue;
    else if (m_selected_queues.transfer_queue_index == m_selected_queues.compute_queue_index)
        m_vk_transfer_queue = m_vk_transfer_queue;
    else
        vkGetDeviceQueue(m_vk_device, m_selected_queues.transfer_queue_index, 0, &m_vk_transfer_queue);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace vk
} // namespace inferno