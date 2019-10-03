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
    VkFormat        find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

private:
    Backend(GLFWwindow* window, bool enable_validation_layers = false);
    VkFormat                 find_depth_format();
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

    static Buffer::Ptr create(Backend::Ptr backend, VkBufferUsageFlags usage, size_t size, VmaMemoryUsage memory_usage, VkFlags create_flags);

    ~Buffer();

    inline VkBuffer handle() { return m_vk_buffer; }
    inline size_t   size() { return m_size; }
    inline void*    mapped_ptr() { return m_mapped_ptr; }
		 
private:
    Buffer(Backend::Ptr backend, VkBufferUsageFlags usage, size_t size, VmaMemoryUsage memory_usage, VkFlags create_flags);

private:
    size_t           m_size;
    void*            m_mapped_ptr       = nullptr;
    VkBuffer         m_vk_buffer        = nullptr;
    VkDeviceMemory   m_vk_device_memory = nullptr;
    VmaAllocator_T*  m_vma_allocator    = nullptr;
    VmaAllocation_T* m_vma_allocation   = nullptr;
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

class ShaderModule : public Object
{
public:
    using Ptr = std::shared_ptr<ShaderModule>;

	static ShaderModule::Ptr create(Backend::Ptr backend, std::vector<uint32_t> spirv);
	
    ~ShaderModule();

	inline VkShaderModule handle() { return m_vk_module; }

private:
	ShaderModule(Backend::Ptr backend, std::vector<uint32_t> spirv);

private:
    VkShaderModule m_vk_module;
};

struct VertexInputStateDesc
{
    VkPipelineVertexInputStateCreateInfo create_info;
    VkVertexInputBindingDescription binding_desc[16];
    VkVertexInputAttributeDescription attribute_desc[16];

	VertexInputStateDesc();
    VertexInputStateDesc& add_binding_desc(uint32_t binding, uint32_t stride, VkVertexInputRate input_rate);
    VertexInputStateDesc& add_attribute_desc(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);
};

struct InputAssemblyStateDesc
{
    VkPipelineInputAssemblyStateCreateInfo create_info;

	InputAssemblyStateDesc();
	InputAssemblyStateDesc& set_flags(VkPipelineInputAssemblyStateCreateFlags flags);
    InputAssemblyStateDesc& set_topology(VkPrimitiveTopology topology);
	InputAssemblyStateDesc& set_primitive_restart_enable(bool primitive_restart_enable);
};

struct TessellationStateDesc
{
    VkPipelineTessellationStateCreateInfo create_info;

	TessellationStateDesc();
	TessellationStateDesc& set_flags(VkPipelineTessellationStateCreateFlags flags);
    TessellationStateDesc& set_patch_control_points(uint32_t patch_control_points);
};

struct RasterizationStateDesc
{
    VkPipelineRasterizationStateCreateInfo create_info;
    VkPipelineRasterizationConservativeStateCreateInfoEXT conservative_raster_create_info;

	RasterizationStateDesc();
    RasterizationStateDesc& set_depth_clamp(VkBool32 value);
    RasterizationStateDesc& set_rasterizer_discard_enable(VkBool32 value);
    RasterizationStateDesc& set_polygon_mode(VkPolygonMode value);
    RasterizationStateDesc& set_cull_mode(VkCullModeFlags value);
    RasterizationStateDesc& set_front_face(VkFrontFace value);
    RasterizationStateDesc& set_depth_bias(VkBool32 value);
    RasterizationStateDesc& set_depth_bias_constant_factor(float value);
    RasterizationStateDesc& set_depth_bias_clamp(float value);
    RasterizationStateDesc& set_depth_bias_slope_factor(float value);
    RasterizationStateDesc& set_line_width(float value);
    RasterizationStateDesc& set_conservative_raster_mode(VkConservativeRasterizationModeEXT value);
    RasterizationStateDesc& set_extra_primitive_overestimation_size(float value);

};

struct MultisampleStateDesc
{
    VkPipelineMultisampleStateCreateInfo create_info;

	MultisampleStateDesc();
	MultisampleStateDesc& set_rasterization_samples(VkSampleCountFlagBits value);
    MultisampleStateDesc& set_sample_shading_enable(VkBool32 value);
    MultisampleStateDesc& set_min_sample_shading(float value);
    MultisampleStateDesc& set_sample_mask(VkSampleMask* value);
    MultisampleStateDesc& set_alpha_to_coverage_enable(VkBool32 value);
    MultisampleStateDesc& set_alpha_to_one_enable(VkBool32 value);
};

struct StencilOpStateDesc
{
    VkStencilOpState create_info;

	StencilOpStateDesc& set_fail_op(VkStencilOp value);
    StencilOpStateDesc& set_pass_op(VkStencilOp value);
    StencilOpStateDesc& set_depth_fail_op(VkStencilOp value);
    StencilOpStateDesc& set_compare_op(VkCompareOp value);
    StencilOpStateDesc& set_compare_mask(uint32_t value);
    StencilOpStateDesc& set_write_mask(uint32_t value);
    StencilOpStateDesc& set_reference(uint32_t value);
};

struct DepthStencilStateDesc
{
    VkPipelineDepthStencilStateCreateInfo create_info;

    DepthStencilStateDesc();
    DepthStencilStateDesc& set_depth_test_enable(VkBool32 value);
    DepthStencilStateDesc& set_depth_write_enable(VkBool32 value);
    DepthStencilStateDesc& set_depth_compare_op(VkCompareOp value);
    DepthStencilStateDesc& set_depth_bounds_test_enable(VkBool32 value);
    DepthStencilStateDesc& set_stencil_test_enable(VkBool32 value);
    DepthStencilStateDesc& set_front(StencilOpStateDesc value);
    DepthStencilStateDesc& set_back(StencilOpStateDesc value);
    DepthStencilStateDesc& set_min_depth_bounds(float value);
    DepthStencilStateDesc& set_max_depth_bounds(float value);
};

struct ColorBlendAttachmentStateDesc
{
    ColorBlendAttachmentStateDesc& set_blend_enable();
    ColorBlendAttachmentStateDesc& set_src_color_blend_factor();
    ColorBlendAttachmentStateDesc& set_dst_color_blend_Factor();
    ColorBlendAttachmentStateDesc& set_color_blend_op();
    ColorBlendAttachmentStateDesc& set_src_alpha_blend_factor();
    ColorBlendAttachmentStateDesc& set_dst_alpha_blend_factor();
    ColorBlendAttachmentStateDesc& set_alpha_blend_op();
    ColorBlendAttachmentStateDesc& set_color_write_mask();
};

//typedef struct VkPipelineColorBlendStateCreateInfo
//{
//    VkStructureType                            sType;
//    const void*                                pNext;
//    VkPipelineColorBlendStateCreateFlags       flags;
//    VkBool32                                   logicOpEnable;
//    VkLogicOp                                  logicOp;
//    uint32_t                                   attachmentCount;
//    const VkPipelineColorBlendAttachmentState* pAttachments;
//    float                                      blendConstants[4];
//} VkPipelineColorBlendStateCreateInfo;

class GraphicsPipeline : public Object
{
public:
    using Ptr = std::shared_ptr<GraphicsPipeline>;

	struct Desc
	{
		uint32_t       shader_module_count = 0;
		VkShaderModule modules[6];

		Desc& add_shader_module(ShaderModule::Ptr shader_module);
	};

	static GraphicsPipeline::Ptr create(Backend::Ptr backend, Desc desc);

	~GraphicsPipeline();

private:
        GraphicsPipeline(Backend::Ptr backend, Desc desc);

private:
    VkPipeline m_vk_pipeline;
};

class ComputePipeline : public Object
{
public:
    using Ptr = std::shared_ptr<ComputePipeline>;

    struct Desc
    {
    };

    static ComputePipeline::Ptr create(Backend::Ptr backend, Desc desc);

    ~ComputePipeline();

private:
    ComputePipeline(Backend::Ptr backend, Desc desc);

private:
    VkPipeline m_vk_pipeline;
};

class Sampler : public Object
{
public:
    using Ptr = std::shared_ptr<Sampler>;

	struct Desc
	{
        VkSamplerCreateFlags     flags;
		VkFilter                 mag_filter;
		VkFilter                 min_filter;
		VkSamplerMipmapMode      mipmap_mode;
		VkSamplerAddressMode     address_mode_u;
		VkSamplerAddressMode     address_mode_v;
		VkSamplerAddressMode     address_mode_w;
		float                    mip_lod_bias;
		VkBool32                 anisotropy_enable;
		float                    max_anisotropy;
		VkBool32                 compare_enable;
		VkCompareOp              compare_op;
		float                    min_lod;
		float                    max_lod;
		VkBorderColor            border_color;
		VkBool32                 unnormalized_coordinates;
	};

    inline VkSampler handle() { return m_vk_sampler; }

    static Sampler::Ptr create(Backend::Ptr backend,  Desc desc);
	
	~Sampler();

private:
    Sampler(Backend::Ptr backend, Desc desc);

private:
    VkSampler m_vk_sampler;
};

class DescriptorSetLayout : public Object
{
public:
    using Ptr = std::shared_ptr<DescriptorSetLayout>;

    struct Desc
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        VkSampler                                 binding_samplers[32][8];

        Desc& add_binding(uint32_t binding, VkDescriptorType descriptor_type, uint32_t descriptor_count, VkShaderStageFlags stage_flags);
        Desc& add_binding(uint32_t binding, VkDescriptorType descriptor_type, uint32_t descriptor_count, VkShaderStageFlags stage_flags, Sampler::Ptr samplers[]);
    };

    static DescriptorSetLayout::Ptr create(Backend::Ptr backend, Desc desc);

    ~DescriptorSetLayout();

    inline VkDescriptorSetLayout handle() { return m_vk_ds_layout; }

private:
    DescriptorSetLayout(Backend::Ptr backend, Desc desc);

private:
    VkDescriptorSetLayout m_vk_ds_layout;
};

class PipelineLayout : public Object
{
public:
    using Ptr = std::shared_ptr<PipelineLayout>;

    struct Desc
    {
        std::vector<DescriptorSetLayout::Ptr> layouts;
        std::vector<VkPushConstantRange>      push_constant_ranges;

        Desc& add_descriptor_set_layout(DescriptorSetLayout::Ptr layout);
        Desc& add_push_constant_range(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size);
    };

    static PipelineLayout::Ptr create(Backend::Ptr backend, Desc desc);

    ~PipelineLayout();

    inline VkPipelineLayout handle() { return m_vk_pipeline_layout; }

private:
    PipelineLayout(Backend::Ptr backend, Desc desc);

private:
    VkPipelineLayout m_vk_pipeline_layout;
};

class DescriptorPool : public Object
{
public:
    using Ptr = std::shared_ptr<DescriptorPool>;

    struct Desc
    {
        uint32_t                          max_sets;
        std::vector<VkDescriptorPoolSize> pool_sizes;

        Desc& set_max_sets(uint32_t num);
        Desc& add_pool_size(VkDescriptorType type, uint32_t descriptor_count);
    };

    static DescriptorPool::Ptr create(Backend::Ptr backend, Desc desc);

    ~DescriptorPool();

    inline VkDescriptorPool handle() { return m_vk_ds_pool; }

private:
    DescriptorPool(Backend::Ptr backend, Desc desc);

private:
    VkDescriptorPool m_vk_ds_pool;
};

class DescriptorSet : public Object
{
public:
    using Ptr = std::shared_ptr<DescriptorSet>;

    static DescriptorSet::Ptr create(Backend::Ptr backend, DescriptorSetLayout::Ptr layout, DescriptorPool::Ptr pool);

    ~DescriptorSet();

    inline VkDescriptorSet handle() { return m_vk_ds; }

private:
    DescriptorSet(Backend::Ptr backend, DescriptorSetLayout::Ptr layout, DescriptorPool::Ptr pool);

private:
    VkDescriptorSet               m_vk_ds;
    std::weak_ptr<DescriptorPool> m_vk_pool;
};

} // namespace vk
} // namespace inferno