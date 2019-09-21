#include "vk.h"
#include "logger.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace inferno
{
namespace vk
{
// -----------------------------------------------------------------------------------------------------------------------------------

bool check_validation_layer_support(std::vector<const char*> layers)
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
			INFERNO_LOG_ERROR("Validation Layer not available: " + std::string(layer_name));
			return false;
		}
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    INFERNO_LOG_ERROR("Vukan Validation Layer: " + std::string(pCallbackData->pMessage));
    return VK_FALSE;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Instance::Instance(std::string app_name, std::vector<const char*> layers)
{
    VkApplicationInfo appInfo;

    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = app_name.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = app_name.c_str();
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = required_extensions(layers.size() > 0);

	VkInstanceCreateInfo create_info;

    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &appInfo;
    create_info.enabledExtensionCount   = extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();
    
	VkDebugUtilsMessengerCreateInfoEXT debug_create_info;

    if (layers.size() > 0)
    {
        create_info.enabledLayerCount  = static_cast<uint32_t>(layers.size());
        create_info.ppEnabledLayerNames = layers.data();
        create_info.pNext               = nullptr;

        /*debug_create_info.sType    = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;

        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;*/
    }
    else
    {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

	if (vkCreateInstance(&create_info, nullptr, &m_vk_instance) != VK_SUCCESS)
	{
        INFERNO_LOG_ERROR("Failed to create Vulkan instance.");
		throw std::runtime_error("Failed to create Vulkan instance.");
	}

	if (layers.size() > 0 && create_debug_utils_messenger(m_vk_instance, &debug_create_info, nullptr, &m_vk_debug_messenger) != VK_SUCCESS)
		INFERNO_LOG_ERROR("Failed to create Vulkan debug messenger.");
}

// -----------------------------------------------------------------------------------------------------------------------------------

Instance::~Instance()
{
    if (m_vk_debug_messenger)
        destroy_debug_utils_messenger(m_vk_instance, m_vk_debug_messenger, nullptr);

    vkDestroyInstance(m_vk_instance, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

std::vector<const char*> Instance::required_extensions(bool enable_validation_layers)
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

VkResult Instance::create_debug_utils_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Instance::destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace vk
} // namespace inferno