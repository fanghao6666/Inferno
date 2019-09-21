#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

struct GLFWwindow;

namespace inferno
{
namespace vk
{
extern bool check_validation_layer_support(std::vector<const char*> layers);

class Instance
{
public:
    Instance(std::string app_name, std::vector<const char*> layers = std::vector<const char*>());
    ~Instance();

private:
    std::vector<const char*> required_extensions(bool enable_validation_layers);
    VkResult create_debug_utils_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    private:
    VkInstance m_vk_instance = nullptr;
    VkDebugUtilsMessengerEXT m_vk_debug_messenger = nullptr;
};
} // namespace vk
} // namespace inferno