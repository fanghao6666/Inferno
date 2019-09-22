#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>

struct GLFWwindow;

namespace inferno
{
namespace vk
{
class CommandBuffer;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> format;
    std::vector<VkPresentModeKHR>   present_modes;
};

struct QueueInfos
{
    // Most ideal queue = 3
    // Second most ideal queue = 2
    // Queue for minimum functionality = 1
    // Not found = 0

    int32_t                 graphics_queue_index     = -1;
    int32_t                 graphics_queue_quality   = 0;
    int32_t                 compute_queue_index      = -1;
    int32_t                 compute_queue_quality    = 0;
    int32_t                 transfer_queue_index     = -1;
    int32_t                 transfer_queue_quality   = 0;
    int32_t                 presentation_queue_index = -1;
    int32_t                 queue_count              = 0;
    VkDeviceQueueCreateInfo infos[32];

	bool asynchronous_compute();
    bool transfer();
};

class Backend
{
public:
    Backend(GLFWwindow* window, bool enable_validation_layers = false);
    ~Backend();

private:
    bool                     check_validation_layer_support(std::vector<const char*> layers);
    bool                     check_device_extension_support(VkPhysicalDevice device);
    void                     query_swap_chain_support(VkPhysicalDevice device, SwapChainSupportDetails& details);
    std::vector<const char*> required_extensions(bool enable_validation_layers);
    VkResult                 create_debug_utils_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void                     destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    bool                     create_surface(GLFWwindow* window);
	bool                     find_physical_device();
    bool                     is_device_suitable(VkPhysicalDevice device, VkPhysicalDeviceType type, QueueInfos& infos);
    bool                     find_queues(VkPhysicalDevice device, QueueInfos& infos);
    bool                     is_queue_compatible(VkQueueFlags current_queue_flags, int32_t graphics, int32_t compute, int32_t transfer);
    bool                     create_logical_device();

private:
    VkInstance               m_vk_instance = nullptr;
    VkDevice                 m_vk_device   = nullptr;
    VkQueue                  m_vk_graphics_queue = nullptr;
    VkQueue                  m_vk_compute_queue   = nullptr;
    VkQueue                  m_vk_transfer_queue  = nullptr;
    VkQueue                  m_vk_presentation_queue = nullptr;
    VkPhysicalDevice         m_vk_physical_device = nullptr;
    VkSurfaceKHR             m_vk_surface         = nullptr;
    VkSwapchainKHR           m_vk_swap_chain      = nullptr;
    VkDebugUtilsMessengerEXT m_vk_debug_messenger = nullptr;
    struct VmaAllocator_T*   m_vk_allocator = nullptr;
    QueueInfos               m_selected_queues;
};

} // namespace vk
} // namespace inferno