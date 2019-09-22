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
    VkSurfaceCapabilitiesKHR   capabilities;
    std::vector<VkSurfaceFormatKHR> format;
    std::vector<VkPresentModeKHR>   present_modes;
};

class Backend
{
public:
    Backend(GLFWwindow* window, bool enable_validation_layers = false);
    ~Backend();

private:
    bool check_validation_layer_support(std::vector<const char*> layers);
    std::vector<const char*> required_extensions(bool enable_validation_layers);
    VkResult                 create_debug_utils_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void                     destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    bool                     find_physical_device();
	bool                     is_device_suitable(VkPhysicalDevice device, VkPhysicalDeviceType type);
    bool                     create_surface(GLFWwindow* window);
    bool                     check_device_extension_support(VkPhysicalDevice device);
    void query_swap_chain_support(VkPhysicalDevice device, SwapChainSupportDetails& details);

    private:
    VkInstance m_vk_instance;
    VkDevice   m_vk_device;
    VkPhysicalDevice m_vk_physical_device;
    VkSurfaceKHR             m_vk_surface;
    VkSwapchainKHR           m_vk_swap_chain;
    VkDebugUtilsMessengerEXT m_vk_debug_messenger; 
    struct VmaAllocator_T*   m_vk_allocator;
};

} // namespace vk
} // namespace inferno