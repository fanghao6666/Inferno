#include "vk.h"
#include "logger.h"
#include "macros.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>

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

Object::Object(Backend::Ptr backend, VkDevice device) :
    m_vk_backend(backend), m_vk_device(device)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::Ptr Image::create(Backend::Ptr backend, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count)
{
    return std::shared_ptr<Image>(new Image(backend, width, height, depth, mip_levels, array_size, format, memory_usage, usage, sample_count));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::Ptr Image::create(VkDevice device, VkImage image, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlags sample_count)
{
    return std::shared_ptr<Image>(new Image(device, image, width, height, format, sample_count));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::Image(Backend::Ptr backend, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count) :
    Object(backend)
{
    m_vma_allocator = backend->allocator();

    VkImageCreateInfo image_info;
    INFERNO_ZERO_MEMORY(image_info);

    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = width;
    image_info.extent.height = height;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = mip_levels;
    image_info.arrayLayers   = array_size;
    image_info.format        = format;
    image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage         = usage;
    image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_info.samples     = sample_count;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationInfo       alloc_info;
    VmaAllocationCreateInfo alloc_create_info;
    INFERNO_ZERO_MEMORY(alloc_create_info);

    alloc_create_info.usage = memory_usage;
    alloc_create_info.flags = 0;

    if (vmaCreateImage(m_vma_allocator, &image_info, &alloc_create_info, &m_vk_image, &m_vma_allocation, &alloc_info) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Image.");
        throw std::runtime_error("(Vulkan) Failed to create Image.");
    }

    m_vk_device_memory = alloc_info.deviceMemory;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::Image(VkDevice device, VkImage image, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlags sample_count) :
    Object(nullptr, device)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::~Image()
{
    if (m_vma_allocator && m_vma_allocation)
        vmaDestroyImage(m_vma_allocator, m_vk_image, m_vma_allocation);
}

// -----------------------------------------------------------------------------------------------------------------------------------

VkImage Image::handle()
{
    return m_vk_image;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ImageView::Ptr ImageView::create(Backend::Ptr backend, Image::Ptr image, VkImageViewType view_type, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count)
{
    return std::shared_ptr<ImageView>(new ImageView(backend, image, view_type, base_mip_level, level_count, base_array_layer, layer_count));
}

// -----------------------------------------------------------------------------------------------------------------------------------

ImageView::Ptr ImageView::create(VkDevice device, Image::Ptr image, VkImageViewType view_type, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count)
{
    return std::shared_ptr<ImageView>(new ImageView(device, image, view_type, base_mip_level, level_count, base_array_layer, layer_count));
}

// -----------------------------------------------------------------------------------------------------------------------------------

ImageView::ImageView(Backend::Ptr backend, Image::Ptr image, VkImageViewType view_type, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count) :
    Object(backend)
{
    VkImageViewCreateInfo info;
    INFERNO_ZERO_MEMORY(info);

    info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image    = image->handle();
    info.viewType = view_type;
    //info.format                          = image->format();
    //info.subresourceRange.aspectMask     = image->aspect_flags();
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(m_vk_device, &info, nullptr, &m_vk_image_view) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

ImageView::ImageView(VkDevice device, Image::Ptr image, VkImageViewType view_type, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count) :
    Object(nullptr, device)
{
    VkImageViewCreateInfo info;
    INFERNO_ZERO_MEMORY(info);
}

// -----------------------------------------------------------------------------------------------------------------------------------

ImageView::~ImageView()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Backend::Ptr Backend::create(GLFWwindow* window, bool enable_validation_layers)
{
    return std::shared_ptr<Backend>(new Backend(window, enable_validation_layers));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Backend::Backend(GLFWwindow* window, bool enable_validation_layers) :
    m_window(window)
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

VkDevice Backend::device()
{
    return m_vk_device;
}

// -----------------------------------------------------------------------------------------------------------------------------------

VmaAllocator_T* Backend::allocator()
{
    return m_vma_allocator;
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
        QueueInfos              infos;
        SwapChainSupportDetails details;

        if (is_device_suitable(device, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, infos, details))
        {
            m_vk_physical_device = device;
            m_selected_queues    = infos;
            m_swapchain_details  = details;
            return true;
        }
    }

    // ...If not, try to find an integrated GPU...
    for (const auto& device : devices)
    {
        QueueInfos              infos;
        SwapChainSupportDetails details;

        if (is_device_suitable(device, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, infos, details))
        {
            m_vk_physical_device = device;
            m_selected_queues    = infos;
            m_swapchain_details  = details;
            return true;
        }
    }

    return false;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::is_device_suitable(VkPhysicalDevice device, VkPhysicalDeviceType type, QueueInfos& infos, SwapChainSupportDetails& details)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    uint32_t vendorId = properties.vendorID;

    if (properties.deviceType == type)
    {
        bool extensions_supported = check_device_extension_support(device);
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
                infos.graphics_queue_index   = i;
                infos.graphics_queue_quality = 3;
            }
            else if (is_queue_compatible(bits, 1, -1, -1))
            {
                // If not, a queue that supports at least graphics.
                infos.graphics_queue_index   = i;
                infos.graphics_queue_quality = 1;
            }
        }

        // Look for a compute queue if the ideal one isn't found yet.
        if (infos.compute_queue_quality != 3)
        {
            if (is_queue_compatible(bits, 0, 1, 0))
            {
                // Ideally, a queue that only supports compute (for asynchronous compute).
                infos.compute_queue_index   = i;
                infos.compute_queue_quality = 3;
            }
            else if (is_queue_compatible(bits, 0, 1, 1))
            {
                // Else, a queue that supports compute and transfer only (might allow asynchronous compute. Have to check).
                infos.compute_queue_index   = i;
                infos.compute_queue_quality = 2;
            }
            else if (is_queue_compatible(bits, -1, 1, -1) && infos.compute_queue_quality == 0)
            {
                // If not, a queue that supports at least compute
                infos.compute_queue_index   = i;
                infos.compute_queue_quality = 1;
            }
        }

        // Look for a Transfer queue if the ideal one isn't found yet.
        if (infos.transfer_queue_quality != 3)
        {
            if (is_queue_compatible(bits, 0, 0, 1))
            {
                // Ideally, a queue that only supports transfer (for DMA).
                infos.transfer_queue_index   = i;
                infos.transfer_queue_quality = 3;
            }
            else if (is_queue_compatible(bits, 0, 1, 1))
            {
                // Else, a queue that supports compute and transfer only.
                infos.transfer_queue_index   = i;
                infos.transfer_queue_quality = 2;
            }
            else if (is_queue_compatible(bits, -1, -1, 1) && infos.transfer_queue_quality == 0)
            {
                // If not, a queue that supports at least graphics
                infos.transfer_queue_index   = i;
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

    presentation_queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    presentation_queue_info.queueFamilyIndex = infos.presentation_queue_index;
    presentation_queue_info.queueCount       = 1;

    VkDeviceQueueCreateInfo graphics_queue_info;
    INFERNO_ZERO_MEMORY(graphics_queue_info);

    graphics_queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphics_queue_info.queueFamilyIndex = infos.graphics_queue_index;
    graphics_queue_info.queueCount       = 1;

    VkDeviceQueueCreateInfo compute_queue_info;
    INFERNO_ZERO_MEMORY(compute_queue_info);

    compute_queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    compute_queue_info.queueFamilyIndex = infos.compute_queue_index;
    compute_queue_info.queueCount       = 1;

    VkDeviceQueueCreateInfo transfer_queue_info;
    INFERNO_ZERO_MEMORY(transfer_queue_info);

    transfer_queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    transfer_queue_info.queueFamilyIndex = infos.transfer_queue_index;
    transfer_queue_info.queueCount       = 1;

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

bool Backend::create_swapchain()
{
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(m_swapchain_details.format);
    VkPresentModeKHR   present_mode   = choose_swap_present_mode(m_swapchain_details.present_modes);
    VkExtent2D         extent         = choose_swap_extent(m_swapchain_details.capabilities);

    uint32_t image_count = m_swapchain_details.capabilities.minImageCount + 1;

    if (m_swapchain_details.capabilities.maxImageCount > 0 && image_count > m_swapchain_details.capabilities.maxImageCount)
        image_count = m_swapchain_details.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create_info;
    INFERNO_ZERO_MEMORY(create_info);

    create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface          = m_vk_surface;
    create_info.minImageCount    = image_count;
    create_info.imageFormat      = surface_format.format;
    create_info.imageColorSpace  = surface_format.colorSpace;
    create_info.imageExtent      = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    m_swap_chain_image_format = surface_format.format;
    m_swap_chain_extent       = extent;

    uint32_t queue_family_indices[] = { (uint32_t)m_selected_queues.graphics_queue_index, (uint32_t)m_selected_queues.presentation_queue_index };

    if (m_selected_queues.presentation_queue_index != m_selected_queues.graphics_queue_index)
    {
        create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices   = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices   = nullptr;
    }

    create_info.preTransform   = m_swapchain_details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode    = present_mode;
    create_info.clipped        = VK_TRUE;
    create_info.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_vk_device, &create_info, nullptr, &m_vk_swap_chain) != VK_SUCCESS)
        return false;

    uint32_t swap_image_count = 0;
    vkGetSwapchainImagesKHR(m_vk_device, m_vk_swap_chain, &swap_image_count, nullptr);
    m_swap_chain_images.resize(swap_image_count);
    m_swap_chain_image_views.resize(swap_image_count);
    m_swap_chain_framebuffers.resize(swap_image_count);

    VkImage images[32];

    if (vkGetSwapchainImagesKHR(m_vk_device, m_vk_swap_chain, &swap_image_count, &images[0]) != VK_SUCCESS)
        return false;

    // @TODO: Create back buffer depth image/image view

    for (int i = 0; i < swap_image_count; i++)
    {
        m_swap_chain_images[i]      = Image::create(m_vk_device, images[i], m_swap_chain_extent.width, m_swap_chain_extent.height, m_swap_chain_image_format, VK_SAMPLE_COUNT_1_BIT);
        m_swap_chain_image_views[i] = ImageView::create(m_vk_device, m_swap_chain_images[i], VK_IMAGE_VIEW_TYPE_2D);

        // @TODO: Create swap chain Framebuffers
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

VkSurfaceFormatKHR Backend::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    for (const auto& available_format : available_formats)
    {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return available_format;
    }

    return available_formats[0];
}

// -----------------------------------------------------------------------------------------------------------------------------------

VkPresentModeKHR Backend::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_modes)
{
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& available_mode : available_modes)
    {
        if (available_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            best_mode = available_mode;
        else if (available_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            best_mode = available_mode;
    }

    return best_mode;
}

// -----------------------------------------------------------------------------------------------------------------------------------

VkExtent2D Backend::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    // Causes macro issue on windows.
#ifdef max
#    undef max
#endif

#ifdef min
#    undef min
#endif

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else
    {
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);

        VkExtent2D actual_extent = { width, height };

        // Make sure the window size is between the surfaces allowed max and min image extents.
        actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));

        return actual_extent;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace vk
} // namespace inferno