#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>

struct GLFWwindow;
struct VmaAllocator_T;
struct VmaAllocation_T;
enum VmaMemoryUsage;

namespace inferno
{
namespace vk
{
class Image;
class ImageView;
class Framebuffer;
class RenderPass;
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
    using Ptr = std::shared_ptr<Backend>;

    static Backend::Ptr create(GLFWwindow* window, bool enable_validation_layers = false);

    ~Backend();

    VkDevice        device();
    VmaAllocator_T* allocator();

private:
    Backend(GLFWwindow* window, bool enable_validation_layers = false);
    bool                     check_validation_layer_support(std::vector<const char*> layers);
    bool                     check_device_extension_support(VkPhysicalDevice device);
    void                     query_swap_chain_support(VkPhysicalDevice device, SwapChainSupportDetails& details);
    std::vector<const char*> required_extensions(bool enable_validation_layers);
    VkResult                 create_debug_utils_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void                     destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    bool                     create_surface(GLFWwindow* window);
    bool                     find_physical_device();
    bool                     is_device_suitable(VkPhysicalDevice device, VkPhysicalDeviceType type, QueueInfos& infos, SwapChainSupportDetails& details);
    bool                     find_queues(VkPhysicalDevice device, QueueInfos& infos);
    bool                     is_queue_compatible(VkQueueFlags current_queue_flags, int32_t graphics, int32_t compute, int32_t transfer);
    bool                     create_logical_device();
    bool                     create_swapchain(std::shared_ptr<Backend> backend);
    void                     create_render_pass(std::shared_ptr<Backend> backend);
    VkSurfaceFormatKHR       choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR         choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_modes);
    VkExtent2D               choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
    GLFWwindow*                               m_window                = nullptr;
    VkInstance                                m_vk_instance           = nullptr;
    VkDevice                                  m_vk_device             = nullptr;
    VkQueue                                   m_vk_graphics_queue     = nullptr;
    VkQueue                                   m_vk_compute_queue      = nullptr;
    VkQueue                                   m_vk_transfer_queue     = nullptr;
    VkQueue                                   m_vk_presentation_queue = nullptr;
    VkPhysicalDevice                          m_vk_physical_device    = nullptr;
    VkSurfaceKHR                              m_vk_surface            = nullptr;
    VkSwapchainKHR                            m_vk_swap_chain         = nullptr;
    VkDebugUtilsMessengerEXT                  m_vk_debug_messenger    = nullptr;
    VmaAllocator_T*                           m_vma_allocator         = nullptr;
    SwapChainSupportDetails                   m_swapchain_details;
    QueueInfos                                m_selected_queues;
    VkFormat                                  m_swap_chain_image_format;
    VkFormat                                  m_swap_chain_depth_format;
    VkExtent2D                                m_swap_chain_extent;
    std::shared_ptr<RenderPass>               m_swap_chain_render_pass;
    std::vector<std::shared_ptr<Image>>       m_swap_chain_images;
    std::vector<std::shared_ptr<ImageView>>   m_swap_chain_image_views;
    std::vector<std::shared_ptr<Framebuffer>> m_swap_chain_framebuffers;
    std::shared_ptr<Image>                    m_swap_chain_depth      = nullptr;
    std::shared_ptr<ImageView>                m_swap_chain_depth_view = nullptr;
};

class Object
{
public:
    Object(Backend::Ptr backend, VkDevice device = nullptr);

protected:
    std::weak_ptr<Backend> m_vk_backend;
    VkDevice               m_vk_device = nullptr;
};

class Image : public Object
{
public:
    using Ptr = std::shared_ptr<Image>;

    static Image::Ptr create(Backend::Ptr backend, VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count, VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED);
    static Image::Ptr create_from_swapchain(Backend::Ptr backend, VkImage image, VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count);

    ~Image();

    inline VkImageType        type() { return m_type; }
    inline VkImage            handle() { return m_vk_image; }
    inline uint32_t           width() { return m_width; }
    inline uint32_t           height() { return m_height; }
    inline uint32_t           depth() { return m_depth; }
    inline uint32_t           mip_levels() { return m_mip_levels; }
    inline uint32_t           array_size() { return m_array_size; }
    inline VkFormat           format() { return m_format; }
    inline VkImageUsageFlags  usage() { return m_usage; }
    inline VmaMemoryUsage     memory_usage() { return m_memory_usage; }
    inline VkSampleCountFlags sample_count() { return m_sample_count; }

private:
    Image(Backend::Ptr backend, VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count, VkImageLayout initial_layout);
    Image(Backend::Ptr backend, VkImage image, VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count);

private:
    uint32_t           m_width;
    uint32_t           m_height;
    uint32_t           m_depth;
    uint32_t           m_mip_levels;
    uint32_t           m_array_size;
    VkFormat           m_format;
    VkImageUsageFlags  m_usage;
    VmaMemoryUsage     m_memory_usage;
    VkSampleCountFlags m_sample_count;
    VkImageType        m_type;
    VkImage            m_vk_image         = nullptr;
    VkDeviceMemory     m_vk_device_memory = nullptr;
    VmaAllocator_T*    m_vma_allocator    = nullptr;
    VmaAllocation_T*   m_vma_allocation   = nullptr;
};

class ImageView : public Object
{
public:
    using Ptr = std::shared_ptr<ImageView>;

    static ImageView::Ptr create(Backend::Ptr backend, Image::Ptr image, VkImageViewType view_type, VkImageAspectFlags aspect_flags, uint32_t base_mip_level = 0, uint32_t level_count = 1, uint32_t base_array_layer = 0, uint32_t layer_count = 1);

    ~ImageView();

	inline VkImageView handle() { return m_vk_image_view; }

private:
    ImageView(Backend::Ptr backend, Image::Ptr image, VkImageViewType view_type, VkImageAspectFlags aspect_flags, uint32_t base_mip_level = 0, uint32_t level_count = 1, uint32_t base_array_layer = 0, uint32_t layer_count = 1);

private:
    VkImageView m_vk_image_view;
};

class RenderPass : public Object
{
public:
    using Ptr = std::shared_ptr<RenderPass>;

    static RenderPass::Ptr create(Backend::Ptr backend, std::vector<VkAttachmentDescription> attachment_descs, std::vector<VkSubpassDescription> subpass_descs, std::vector<VkSubpassDependency> subpass_deps);
    ~RenderPass();
    
	inline VkRenderPass handle() { return m_vk_render_pass; }

private:
    RenderPass(Backend::Ptr backend, std::vector<VkAttachmentDescription> attachment_descs, std::vector<VkSubpassDescription> subpass_descs, std::vector<VkSubpassDependency> subpass_deps);

private:
    VkRenderPass m_vk_render_pass = nullptr;
};

class Framebuffer : public Object
{
public:
    using Ptr = std::shared_ptr<Framebuffer>;

	static Framebuffer::Ptr create(Backend::Ptr backend, RenderPass::Ptr render_pass, std::vector<ImageView::Ptr> views, uint32_t width, uint32_t height, uint32_t layers);

    ~Framebuffer();

	inline VkFramebuffer handle() { return m_vk_framebuffer; }

private:
	Framebuffer(Backend::Ptr backend, RenderPass::Ptr render_pass, std::vector<ImageView::Ptr> views, uint32_t width, uint32_t height, uint32_t layers);

private:
    VkFramebuffer m_vk_framebuffer;
};

class Buffer : public Object
{
public:
    using Ptr = std::shared_ptr<Buffer>;
};

class CommandPool : public Object
{
public:
    using Ptr = std::shared_ptr<CommandPool>;

    static CommandPool::Ptr create(Backend::Ptr backend, uint32_t queue_family_index);

    ~CommandPool();

    inline VkCommandPool handle() { return m_vk_pool; }

private:
    CommandPool(Backend::Ptr backend, uint32_t queue_family_index);

private:
    VkCommandPool m_vk_pool = nullptr;
};

class CommandBuffer : public Object
{
public:
    using Ptr = std::shared_ptr<CommandBuffer>;

    static CommandBuffer::Ptr create(Backend::Ptr backend, CommandPool::Ptr pool);

    ~CommandBuffer();

    void                   reset();
    inline VkCommandBuffer handle() { return m_vk_command_buffer; }

private:
    CommandBuffer(Backend::Ptr backend, CommandPool::Ptr pool);

private:
    VkCommandBuffer            m_vk_command_buffer;
    std::weak_ptr<CommandPool> m_vk_pool;
};

} // namespace vk
} // namespace inferno