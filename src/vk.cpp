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
    m_vk_backend(backend)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::Ptr Image::create(Backend::Ptr backend, VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count, VkImageLayout initial_layout)
{
    return std::shared_ptr<Image>(new Image(backend, type, width, height, depth, mip_levels, array_size, format, memory_usage, usage, sample_count, initial_layout));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::Ptr Image::create_from_swapchain(Backend::Ptr backend, VkImage image, VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count)
{
    return std::shared_ptr<Image>(new Image(backend, image, type, width, height, depth, mip_levels, array_size, format, memory_usage, usage, sample_count));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::Image(Backend::Ptr backend, VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count, VkImageLayout initial_layout) :
    Object(backend), m_type(type), m_width(width), m_height(height), m_depth(depth), m_mip_levels(mip_levels), m_array_size(array_size), m_format(format), m_memory_usage(memory_usage), m_sample_count(sample_count)
{
    m_vma_allocator = backend->allocator();

    VkImageCreateInfo image_info;
    INFERNO_ZERO_MEMORY(image_info);

    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = type;
    image_info.extent.width  = width;
    image_info.extent.height = height;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = mip_levels;
    image_info.arrayLayers   = array_size;
    image_info.format        = format;
    image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = initial_layout;
    image_info.usage         = usage;
    image_info.samples       = sample_count;
    image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

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

Image::Image(Backend::Ptr backend, VkImage image, VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_size, VkFormat format, VmaMemoryUsage memory_usage, VkImageUsageFlagBits usage, VkSampleCountFlagBits sample_count) :
    Object(backend), m_vk_image(image), m_type(type), m_width(width), m_height(height), m_depth(depth), m_mip_levels(mip_levels), m_array_size(array_size), m_format(format), m_memory_usage(memory_usage), m_sample_count(sample_count)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Image::~Image()
{
    if (m_vma_allocator && m_vma_allocation)
        vmaDestroyImage(m_vma_allocator, m_vk_image, m_vma_allocation);
}

// -----------------------------------------------------------------------------------------------------------------------------------

ImageView::Ptr ImageView::create(Backend::Ptr backend, Image::Ptr image, VkImageViewType view_type, VkImageAspectFlags aspect_flags, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count)
{
    return std::shared_ptr<ImageView>(new ImageView(backend, image, view_type, aspect_flags, base_mip_level, level_count, base_array_layer, layer_count));
}

// -----------------------------------------------------------------------------------------------------------------------------------

ImageView::ImageView(Backend::Ptr backend, Image::Ptr image, VkImageViewType view_type, VkImageAspectFlags aspect_flags, uint32_t base_mip_level, uint32_t level_count, uint32_t base_array_layer, uint32_t layer_count) :
    Object(backend)
{
    VkImageViewCreateInfo info;
    INFERNO_ZERO_MEMORY(info);

    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = image->handle();
    info.viewType                        = view_type;
    info.format                          = image->format();
    info.subresourceRange.aspectMask     = aspect_flags;
    info.subresourceRange.baseMipLevel   = base_mip_level;
    info.subresourceRange.levelCount     = level_count;
    info.subresourceRange.baseArrayLayer = base_array_layer;
    info.subresourceRange.layerCount     = layer_count;

    if (vkCreateImageView(backend->device(), &info, nullptr, &m_vk_image_view) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Image View.");
        throw std::runtime_error("(Vulkan) Failed to create Image View.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

ImageView::~ImageView()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyImageView(backend->device(), m_vk_image_view, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

RenderPass::Ptr RenderPass::create(Backend::Ptr backend, std::vector<VkAttachmentDescription> attachment_descs, std::vector<VkSubpassDescription> subpass_descs, std::vector<VkSubpassDependency> subpass_deps)
{
    return std::shared_ptr<RenderPass>(new RenderPass(backend, attachment_descs, subpass_descs, subpass_deps));
}

// -----------------------------------------------------------------------------------------------------------------------------------

RenderPass::RenderPass(Backend::Ptr backend, std::vector<VkAttachmentDescription> attachment_descs, std::vector<VkSubpassDescription> subpass_descs, std::vector<VkSubpassDependency> subpass_deps) :
    Object(backend)
{
    VkRenderPassCreateInfo render_pass_info;
    INFERNO_ZERO_MEMORY(render_pass_info);

    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = attachment_descs.size();
    render_pass_info.pAttachments    = attachment_descs.data();
    render_pass_info.subpassCount    = subpass_descs.size();
    render_pass_info.pSubpasses      = subpass_descs.data();
    render_pass_info.dependencyCount = subpass_deps.size();
    render_pass_info.pDependencies   = subpass_deps.data();

    if (vkCreateRenderPass(backend->device(), &render_pass_info, nullptr, &m_vk_render_pass) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Render Pass.");
        throw std::runtime_error("(Vulkan) Failed to create Render Pass.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

RenderPass::~RenderPass()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyRenderPass(backend->device(), m_vk_render_pass, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Framebuffer::Ptr Framebuffer::create(Backend::Ptr backend, RenderPass::Ptr render_pass, std::vector<ImageView::Ptr> views, uint32_t width, uint32_t height, uint32_t layers)
{
    return std::shared_ptr<Framebuffer>(new Framebuffer(backend, render_pass, views, width, height, layers));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Framebuffer::Framebuffer(Backend::Ptr backend, RenderPass::Ptr render_pass, std::vector<ImageView::Ptr> views, uint32_t width, uint32_t height, uint32_t layers) :
    Object(backend)
{
    std::vector<VkImageView> attachments(views.size());

    for (int i = 0; i < attachments.size(); i++)
        attachments[i] = views[i]->handle();

    VkFramebufferCreateInfo frameBuffer_create_info;
    INFERNO_ZERO_MEMORY(frameBuffer_create_info);

    frameBuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBuffer_create_info.pNext           = NULL;
    frameBuffer_create_info.renderPass      = render_pass->handle();
    frameBuffer_create_info.attachmentCount = views.size();
    frameBuffer_create_info.pAttachments    = attachments.data();
    frameBuffer_create_info.width           = width;
    frameBuffer_create_info.height          = height;
    frameBuffer_create_info.layers          = layers;

    if (vkCreateFramebuffer(backend->device(), &frameBuffer_create_info, nullptr, &m_vk_framebuffer) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Framebuffer.");
        throw std::runtime_error("(Vulkan) Failed to create Framebuffer.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Framebuffer::~Framebuffer()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyFramebuffer(backend->device(), m_vk_framebuffer, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Buffer::Ptr Buffer::create(Backend::Ptr backend, VkBufferUsageFlags usage, size_t size, VmaMemoryUsage memory_usage, VkFlags create_flags)
{
    return std::shared_ptr<Buffer>(new Buffer(backend, usage, size, memory_usage, create_flags));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Buffer::Buffer(Backend::Ptr backend, VkBufferUsageFlags usage, size_t size, VmaMemoryUsage memory_usage, VkFlags create_flags) :
    Object(backend), m_size(size)
{
    m_vma_allocator = backend->allocator();

    VkBufferCreateInfo buffer_info;
    INFERNO_ZERO_MEMORY(buffer_info);

    buffer_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size                  = size;
    buffer_info.usage                 = usage;
    buffer_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 0;
    buffer_info.pQueueFamilyIndices   = nullptr;

    VkMemoryPropertyFlags memory_prop_flags = 0;
    VkBufferUsageFlags    usage_flags       = usage;

    if (memory_usage == VMA_MEMORY_USAGE_CPU_ONLY)
    {
        memory_prop_flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        memory_prop_flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        usage_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    else if (memory_usage == VMA_MEMORY_USAGE_GPU_ONLY)
    {
        memory_prop_flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    else if (memory_usage == VMA_MEMORY_USAGE_CPU_TO_GPU)
    {
        memory_prop_flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        memory_prop_flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else if (memory_usage == VMA_MEMORY_USAGE_GPU_TO_CPU)
        memory_prop_flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    VmaAllocationInfo vma_alloc_info;

    VmaAllocationCreateInfo alloc_create_info;
    INFERNO_ZERO_MEMORY(alloc_create_info);

    alloc_create_info.usage          = memory_usage;
    alloc_create_info.flags          = create_flags;
    alloc_create_info.requiredFlags  = memory_prop_flags;
    alloc_create_info.preferredFlags = 0;
    alloc_create_info.memoryTypeBits = 0;
    alloc_create_info.pool           = VK_NULL_HANDLE;

    if (vmaCreateBuffer(m_vma_allocator, &buffer_info, &alloc_create_info, &m_vk_buffer, &m_vma_allocation, &vma_alloc_info) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Buffer.");
        throw std::runtime_error("(Vulkan) Failed to create Buffer.");
    }

    m_vk_device_memory = vma_alloc_info.deviceMemory;

    if (create_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
        m_mapped_ptr = vma_alloc_info.pMappedData;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Buffer::~Buffer()
{
    vmaDestroyBuffer(m_vma_allocator, m_vk_buffer, m_vma_allocation);
}

// -----------------------------------------------------------------------------------------------------------------------------------

CommandPool::Ptr CommandPool::create(Backend::Ptr backend, uint32_t queue_family_index)
{
    return std::shared_ptr<CommandPool>(new CommandPool(backend, queue_family_index));
}

// -----------------------------------------------------------------------------------------------------------------------------------

CommandPool::CommandPool(Backend::Ptr backend, uint32_t queue_family_index) :
    Object(backend)
{
    VkCommandPoolCreateInfo pool_info;
    INFERNO_ZERO_MEMORY(pool_info);

    pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_index;

    if (vkCreateCommandPool(backend->device(), &pool_info, nullptr, &m_vk_pool) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Command Pool.");
        throw std::runtime_error("(Vulkan) Failed to create Command Pool.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

CommandPool::~CommandPool()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyCommandPool(backend->device(), m_vk_pool, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

CommandBuffer::CommandBuffer(Backend::Ptr backend, CommandPool::Ptr pool) :
    Object(backend)
{
    m_vk_pool = pool;

    VkCommandBufferAllocateInfo alloc_info;
    INFERNO_ZERO_MEMORY(alloc_info);

    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool        = pool->handle();
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(backend->device(), &alloc_info, &m_vk_command_buffer) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to allocate Command Buffer.");
        throw std::runtime_error("(Vulkan) Failed to allocate Command Buffer.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

CommandBuffer::Ptr CommandBuffer::create(Backend::Ptr backend, CommandPool::Ptr pool)
{
    return std::shared_ptr<CommandBuffer>(new CommandBuffer(backend, pool));
}

// -----------------------------------------------------------------------------------------------------------------------------------

CommandBuffer::~CommandBuffer()
{
    if (m_vk_backend.expired() || m_vk_pool.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();
    auto pool    = m_vk_pool.lock();

    vkFreeCommandBuffers(backend->device(), pool->handle(), 1, &m_vk_command_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void CommandBuffer::reset()
{
    vkResetCommandBuffer(m_vk_command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
}

// -----------------------------------------------------------------------------------------------------------------------------------

ShaderModule::Ptr ShaderModule::create(Backend::Ptr backend, std::vector<uint32_t> spirv)
{
    return std::shared_ptr<ShaderModule>(new ShaderModule(backend, spirv));
}

// -----------------------------------------------------------------------------------------------------------------------------------

ShaderModule::ShaderModule(Backend::Ptr backend, std::vector<uint32_t> spirv) :
    Object(backend)
{
    VkShaderModuleCreateInfo create_info;
    INFERNO_ZERO_MEMORY(create_info);

    create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = spirv.size();
    create_info.pCode    = reinterpret_cast<const uint32_t*>(spirv.data());

    VkShaderModule shader_module;

    if (vkCreateShaderModule(backend->device(), &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create shader module.");
        throw std::runtime_error("(Vulkan) Failed to create shader module.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

ShaderModule::~ShaderModule()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyShaderModule(backend->device(), m_vk_module, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexInputStateDesc::VertexInputStateDesc()
{
    INFERNO_ZERO_MEMORY(create_info);

    create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    create_info.pVertexAttributeDescriptions = &attribute_desc[0];
    create_info.pVertexBindingDescriptions   = &binding_desc[0];
}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexInputStateDesc& VertexInputStateDesc::add_binding_desc(uint32_t binding, uint32_t stride, VkVertexInputRate input_rate)
{
    binding_desc[create_info.vertexBindingDescriptionCount++] = { binding, stride, input_rate };
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexInputStateDesc& VertexInputStateDesc::add_attribute_desc(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
{
    attribute_desc[create_info.vertexAttributeDescriptionCount++] = { location, binding, format, offset };
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

InputAssemblyStateDesc::InputAssemblyStateDesc()
{
    INFERNO_ZERO_MEMORY(create_info);

    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
}

// -----------------------------------------------------------------------------------------------------------------------------------

InputAssemblyStateDesc& InputAssemblyStateDesc::set_flags(VkPipelineInputAssemblyStateCreateFlags flags)
{
    create_info.flags = flags;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

InputAssemblyStateDesc& InputAssemblyStateDesc::set_topology(VkPrimitiveTopology topology)
{
    create_info.topology = topology;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

InputAssemblyStateDesc& InputAssemblyStateDesc::set_primitive_restart_enable(bool primitive_restart_enable)
{
    create_info.primitiveRestartEnable = primitive_restart_enable;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

TessellationStateDesc::TessellationStateDesc()
{
    INFERNO_ZERO_MEMORY(create_info);

    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
}

// -----------------------------------------------------------------------------------------------------------------------------------

TessellationStateDesc& TessellationStateDesc::set_flags(VkPipelineTessellationStateCreateFlags flags)
{
    create_info.flags = flags;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

TessellationStateDesc& TessellationStateDesc::set_patch_control_points(uint32_t patch_control_points)
{
    create_info.patchControlPoints = patch_control_points;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc::RasterizationStateDesc()
{
    INFERNO_ZERO_MEMORY(create_info);
    INFERNO_ZERO_MEMORY(conservative_raster_create_info);

    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    conservative_raster_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_depth_clamp(VkBool32 value)
{
    create_info.depthClampEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_rasterizer_discard_enable(VkBool32 value)
{
    create_info.rasterizerDiscardEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_polygon_mode(VkPolygonMode value)
{
    create_info.polygonMode = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_cull_mode(VkCullModeFlags value)
{
    create_info.cullMode = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_front_face(VkFrontFace value)
{
    create_info.frontFace = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_depth_bias(VkBool32 value)
{
    create_info.depthBiasEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_depth_bias_constant_factor(float value)
{
    create_info.depthBiasConstantFactor = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_depth_bias_clamp(float value)
{
    create_info.depthBiasClamp = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_depth_bias_slope_factor(float value)
{
    create_info.depthBiasSlopeFactor = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_line_width(float value)
{
    create_info.lineWidth = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_conservative_raster_mode(VkConservativeRasterizationModeEXT value)
{
    if (value != VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT)
        create_info.pNext = &conservative_raster_create_info;

    conservative_raster_create_info.conservativeRasterizationMode = value;

    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RasterizationStateDesc& RasterizationStateDesc::set_extra_primitive_overestimation_size(float value)
{
    conservative_raster_create_info.extraPrimitiveOverestimationSize = value;

    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

MultisampleStateDesc::MultisampleStateDesc()
{
    INFERNO_ZERO_MEMORY(create_info);

    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
}

// -----------------------------------------------------------------------------------------------------------------------------------

MultisampleStateDesc& MultisampleStateDesc::set_rasterization_samples(VkSampleCountFlagBits value)
{
    create_info.rasterizationSamples = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

MultisampleStateDesc& MultisampleStateDesc::set_sample_shading_enable(VkBool32 value)
{
    create_info.sampleShadingEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

MultisampleStateDesc& MultisampleStateDesc::set_min_sample_shading(float value)
{
    create_info.minSampleShading = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

MultisampleStateDesc& MultisampleStateDesc::set_sample_mask(VkSampleMask* value)
{
    create_info.pSampleMask = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

MultisampleStateDesc& MultisampleStateDesc::set_alpha_to_coverage_enable(VkBool32 value)
{
    create_info.alphaToCoverageEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

MultisampleStateDesc& MultisampleStateDesc::set_alpha_to_one_enable(VkBool32 value)
{
    create_info.alphaToOneEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

StencilOpStateDesc& StencilOpStateDesc::set_fail_op(VkStencilOp value)
{
    create_info.failOp = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

StencilOpStateDesc& StencilOpStateDesc::set_pass_op(VkStencilOp value)
{
    create_info.passOp = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

StencilOpStateDesc& StencilOpStateDesc::set_depth_fail_op(VkStencilOp value)
{
    create_info.depthFailOp = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

StencilOpStateDesc& StencilOpStateDesc::set_compare_op(VkCompareOp value)
{
    create_info.compareOp = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

StencilOpStateDesc& StencilOpStateDesc::set_compare_mask(uint32_t value)
{
    create_info.compareMask = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

StencilOpStateDesc& StencilOpStateDesc::set_write_mask(uint32_t value)
{
    create_info.writeMask = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

StencilOpStateDesc& StencilOpStateDesc::set_reference(uint32_t value)
{
    create_info.reference = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc::DepthStencilStateDesc()
{
    INFERNO_ZERO_MEMORY(create_info);

    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_depth_test_enable(VkBool32 value)
{
    create_info.depthTestEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_depth_write_enable(VkBool32 value)
{
    create_info.depthWriteEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_depth_compare_op(VkCompareOp value)
{
    create_info.depthCompareOp = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_depth_bounds_test_enable(VkBool32 value)
{
    create_info.depthBoundsTestEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_stencil_test_enable(VkBool32 value)
{
    create_info.stencilTestEnable = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_front(StencilOpStateDesc value)
{
    create_info.front = value.create_info;
    return *this;

}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_back(StencilOpStateDesc value)
{
    create_info.back = value.create_info;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_min_depth_bounds(float value)
{
    create_info.minDepthBounds = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DepthStencilStateDesc& DepthStencilStateDesc::set_max_depth_bounds(float value)
{
    create_info.maxDepthBounds = value;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ColorBlendAttachmentStateDesc& ColorBlendAttachmentStateDesc::set_blend_enable()
{
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ColorBlendAttachmentStateDesc& ColorBlendAttachmentStateDesc::set_src_color_blend_factor()
{
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ColorBlendAttachmentStateDesc& ColorBlendAttachmentStateDesc::set_dst_color_blend_Factor()
{
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ColorBlendAttachmentStateDesc& ColorBlendAttachmentStateDesc::set_color_blend_op()
{
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ColorBlendAttachmentStateDesc& ColorBlendAttachmentStateDesc::set_src_alpha_blend_factor()
{
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ColorBlendAttachmentStateDesc& ColorBlendAttachmentStateDesc::set_dst_alpha_blend_factor()
{
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ColorBlendAttachmentStateDesc& ColorBlendAttachmentStateDesc::set_alpha_blend_op()
{
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ColorBlendAttachmentStateDesc& ColorBlendAttachmentStateDesc::set_color_write_mask()
{
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

GraphicsPipeline::Desc& GraphicsPipeline::Desc::add_shader_module(ShaderModule::Ptr shader_module)
{
    modules[shader_module_count++] = shader_module->handle();
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

GraphicsPipeline::Ptr GraphicsPipeline::create(Backend::Ptr backend, Desc desc)
{
    return std::shared_ptr<GraphicsPipeline>(new GraphicsPipeline(backend, desc));
}

// -----------------------------------------------------------------------------------------------------------------------------------

GraphicsPipeline::GraphicsPipeline(Backend::Ptr backend, Desc desc) :
    Object(backend)
{
    VkGraphicsPipelineCreateInfo pipeline_info;
    INFERNO_ZERO_MEMORY(pipeline_info);

    if (vkCreateGraphicsPipelines(backend->device(), nullptr, 1, &pipeline_info, nullptr, &m_vk_pipeline) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Graphics Pipeline.");
        throw std::runtime_error("(Vulkan) Failed to create Graphics Pipeline.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

GraphicsPipeline::~GraphicsPipeline()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyPipeline(backend->device(), m_vk_pipeline, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

ComputePipeline::Ptr ComputePipeline::create(Backend::Ptr backend, Desc desc)
{
    return std::shared_ptr<ComputePipeline>(new ComputePipeline(backend, desc));
}

// -----------------------------------------------------------------------------------------------------------------------------------

ComputePipeline::ComputePipeline(Backend::Ptr backend, Desc desc) :
    Object(backend)
{
    VkComputePipelineCreateInfo pipeline_info;
    INFERNO_ZERO_MEMORY(pipeline_info);

    if (vkCreateComputePipelines(backend->device(), nullptr, 1, &pipeline_info, nullptr, &m_vk_pipeline) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Compute Pipeline.");
        throw std::runtime_error("(Vulkan) Failed to create Compute Pipeline.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

ComputePipeline::~ComputePipeline()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyPipeline(backend->device(), m_vk_pipeline, nullptr);
}
// -----------------------------------------------------------------------------------------------------------------------------------

Sampler::Ptr Sampler::create(Backend::Ptr backend, Desc desc)
{
    return std::shared_ptr<Sampler>(new Sampler(backend, desc));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Sampler::Sampler(Backend::Ptr backend, Desc desc) :
    Object(backend)
{
    VkSamplerCreateInfo info;
    INFERNO_ZERO_MEMORY(info);

    info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.flags                   = desc.flags;
    info.magFilter               = desc.mag_filter;
    info.minFilter               = desc.min_filter;
    info.mipmapMode              = desc.mipmap_mode;
    info.addressModeU            = desc.address_mode_u;
    info.addressModeV            = desc.address_mode_v;
    info.addressModeW            = desc.address_mode_w;
    info.mipLodBias              = desc.mip_lod_bias;
    info.anisotropyEnable        = desc.anisotropy_enable;
    info.maxAnisotropy           = desc.max_anisotropy;
    info.compareEnable           = desc.compare_enable;
    info.compareOp               = desc.compare_op;
    info.minLod                  = desc.min_lod;
    info.maxLod                  = desc.max_lod;
    info.borderColor             = desc.border_color;
    info.unnormalizedCoordinates = desc.unnormalized_coordinates;

    if (vkCreateSampler(backend->device(), &info, nullptr, &m_vk_sampler) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create sampler.");
        throw std::runtime_error("(Vulkan) Failed to create sampler.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Sampler::~Sampler()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroySampler(backend->device(), m_vk_sampler, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorSetLayout::Desc& DescriptorSetLayout::Desc::add_binding(uint32_t binding, VkDescriptorType descriptor_type, uint32_t descriptor_count, VkShaderStageFlags stage_flags)
{
    bindings.push_back({ binding, descriptor_type, descriptor_count, stage_flags, nullptr });
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorSetLayout::Desc& DescriptorSetLayout::Desc::add_binding(uint32_t binding, VkDescriptorType descriptor_type, uint32_t descriptor_count, VkShaderStageFlags stage_flags, Sampler::Ptr samplers[])
{
    for (int i = 0; i < descriptor_count; i++)
        binding_samplers[binding][i] = samplers[i]->handle();

    bindings.push_back({ binding, descriptor_type, descriptor_count, stage_flags, &binding_samplers[binding][0] });
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorSetLayout::Ptr DescriptorSetLayout::create(Backend::Ptr backend, Desc desc)
{
    return std::shared_ptr<DescriptorSetLayout>(new DescriptorSetLayout(backend, desc));
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorSetLayout::DescriptorSetLayout(Backend::Ptr backend, Desc desc) :
    Object(backend)
{
    VkDescriptorSetLayoutCreateInfo layout_info;
    INFERNO_ZERO_MEMORY(layout_info);

    layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = desc.bindings.size();
    layout_info.pBindings    = desc.bindings.data();

    if (vkCreateDescriptorSetLayout(backend->device(), &layout_info, nullptr, &m_vk_ds_layout) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Descriptor Set Layout.");
        throw std::runtime_error("(Vulkan) Failed to create Descriptor Set Layout.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyDescriptorSetLayout(backend->device(), m_vk_ds_layout, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

PipelineLayout::Desc& PipelineLayout::Desc::add_descriptor_set_layout(DescriptorSetLayout::Ptr layout)
{
    layouts.push_back(layout);
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

PipelineLayout::Desc& PipelineLayout::Desc::add_push_constant_range(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size)
{
    push_constant_ranges.push_back({ stage_flags, offset, size });
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

PipelineLayout::Ptr PipelineLayout::create(Backend::Ptr backend, Desc desc)
{
    return std::shared_ptr<PipelineLayout>(new PipelineLayout(backend, desc));
}

// -----------------------------------------------------------------------------------------------------------------------------------

PipelineLayout::PipelineLayout(Backend::Ptr backend, Desc desc) :
    Object(backend)
{
    std::vector<VkDescriptorSetLayout> vk_layouts(desc.layouts.size());

    for (uint32_t i = 0; i < desc.layouts.size(); i++)
        vk_layouts[i] = desc.layouts[i]->handle();

    VkPipelineLayoutCreateInfo info;
    INFERNO_ZERO_MEMORY(info);

    info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pushConstantRangeCount = desc.push_constant_ranges.size();
    info.pPushConstantRanges    = desc.push_constant_ranges.data();
    info.setLayoutCount         = desc.layouts.size();
    info.pSetLayouts            = vk_layouts.data();

    if (vkCreatePipelineLayout(backend->device(), &info, nullptr, &m_vk_pipeline_layout) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create pipeline layout.");
        throw std::runtime_error("(Vulkan) Failed to create pipeline layout.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

PipelineLayout::~PipelineLayout()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyPipelineLayout(backend->device(), m_vk_pipeline_layout, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorPool::Desc& DescriptorPool::Desc::set_max_sets(uint32_t num)
{
    max_sets = num;
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorPool::Desc& DescriptorPool::Desc::add_pool_size(VkDescriptorType type, uint32_t descriptor_count)
{
    pool_sizes.push_back({ type, descriptor_count });
    return *this;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorPool::Ptr DescriptorPool::create(Backend::Ptr backend, Desc desc)
{
    return std::shared_ptr<DescriptorPool>(new DescriptorPool(backend, desc));
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorPool::DescriptorPool(Backend::Ptr backend, Desc desc) :
    Object(backend)
{
    VkDescriptorPoolCreateInfo pool_info;
    INFERNO_ZERO_MEMORY(pool_info);

    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(desc.pool_sizes.size());
    pool_info.pPoolSizes    = desc.pool_sizes.data();
    pool_info.maxSets       = desc.max_sets;

    if (vkCreateDescriptorPool(backend->device(), &pool_info, nullptr, &m_vk_ds_pool) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create descriptor pool.");
        throw std::runtime_error("(Vulkan) Failed to create descriptor pool.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorPool::~DescriptorPool()
{
    if (m_vk_backend.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();

    vkDestroyDescriptorPool(backend->device(), m_vk_ds_pool, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorSet::Ptr DescriptorSet::create(Backend::Ptr backend, DescriptorSetLayout::Ptr layout, DescriptorPool::Ptr pool)
{
    return std::shared_ptr<DescriptorSet>(new DescriptorSet(backend, layout, pool));
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorSet::DescriptorSet(Backend::Ptr backend, DescriptorSetLayout::Ptr layout, DescriptorPool::Ptr pool) :
    Object(backend)
{
    m_vk_pool = pool;

    VkDescriptorSetAllocateInfo info;
    INFERNO_ZERO_MEMORY(info);

    VkDescriptorSetLayout vk_layout = layout->handle();

    info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool     = pool->handle();
    info.descriptorSetCount = 1;
    info.pSetLayouts        = &vk_layout;

    if (vkAllocateDescriptorSets(backend->device(), &info, &m_vk_ds) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to allocate descriptor set.");
        throw std::runtime_error("(Vulkan) Failed to allocate descriptor set.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

DescriptorSet::~DescriptorSet()
{
    if (m_vk_backend.expired() || m_vk_pool.expired())
    {
        INFERNO_LOG_FATAL("(Vulkan) Destructing after Device.");
        throw std::runtime_error("(Vulkan) Destructing after Device.");
    }

    auto backend = m_vk_backend.lock();
    auto pool    = m_vk_pool.lock();

    vkFreeDescriptorSets(backend->device(), pool->handle(), 1, &m_vk_ds);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Backend::Ptr Backend::create(GLFWwindow* window, bool enable_validation_layers)
{
    Backend*                 backend        = new Backend(window, enable_validation_layers);
    std::shared_ptr<Backend> backend_shared = std::shared_ptr<Backend>(backend);
    backend->create_swapchain(backend_shared);

    return backend_shared;
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

    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice         = m_vk_physical_device;
    allocator_info.device                 = m_vk_device;

    if (vmaCreateAllocator(&allocator_info, &m_vma_allocator) != VK_SUCCESS)
    {
        INFERNO_LOG_FATAL("(Vulkan) Failed to create Allocator.");
        throw std::runtime_error("(Vulkan) Failed to create Allocator.");
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Backend::~Backend()
{
    for (int i = 0; i < m_swap_chain_images.size(); i++)
    {
        m_swap_chain_framebuffers[i].reset();
        m_swap_chain_image_views[i].reset();
    }

    m_swap_chain_render_pass.reset();
    m_swap_chain_depth_view.reset();
    m_swap_chain_depth.reset();

    if (m_vk_debug_messenger)
        destroy_debug_utils_messenger(m_vk_instance, m_vk_debug_messenger, nullptr);

    vkDestroySwapchainKHR(m_vk_device, m_vk_swap_chain, nullptr);
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

VkFormat Backend::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_vk_physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

// -----------------------------------------------------------------------------------------------------------------------------------

VkFormat Backend::find_depth_format()
{
    return find_supported_format({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
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

// -----------------------------------------------------------------------------------------------------------------------------------

bool Backend::create_swapchain(std::shared_ptr<Backend> backend)
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

    m_swap_chain_depth_format = find_depth_format();

    m_swap_chain_depth = Image::create(backend,
                                       VK_IMAGE_TYPE_2D,
                                       m_swap_chain_extent.width,
                                       m_swap_chain_extent.height,
                                       1,
                                       1,
                                       1,
                                       m_swap_chain_depth_format,
                                       VMA_MEMORY_USAGE_GPU_ONLY,
                                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                       VK_SAMPLE_COUNT_1_BIT);

    m_swap_chain_depth_view = ImageView::create(backend, m_swap_chain_depth, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);

    create_render_pass(backend);

    std::vector<ImageView::Ptr> views(2);

    views[1] = m_swap_chain_depth_view;

    for (int i = 0; i < swap_image_count; i++)
    {
        m_swap_chain_images[i]      = Image::create_from_swapchain(backend, images[i], VK_IMAGE_TYPE_2D, m_swap_chain_extent.width, m_swap_chain_extent.height, 1, 1, 1, m_swap_chain_image_format, VMA_MEMORY_USAGE_UNKNOWN, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT);
        m_swap_chain_image_views[i] = ImageView::create(backend, m_swap_chain_images[i], VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

        views[0] = m_swap_chain_image_views[i];

        m_swap_chain_framebuffers[i] = Framebuffer::create(backend, m_swap_chain_render_pass, views, m_swap_chain_extent.width, m_swap_chain_extent.height, 1);
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Backend::create_render_pass(std::shared_ptr<Backend> backend)
{
    std::vector<VkAttachmentDescription> attachments(2);

    // Color attachment
    attachments[0].format         = m_swap_chain_image_format;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    attachments[1].format         = m_swap_chain_depth_format;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_reference;
    color_reference.attachment = 0;
    color_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_reference;
    depth_reference.attachment = 1;
    depth_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::vector<VkSubpassDescription> subpass_description(1);

    subpass_description[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description[0].colorAttachmentCount    = 1;
    subpass_description[0].pColorAttachments       = &color_reference;
    subpass_description[0].pDepthStencilAttachment = &depth_reference;
    subpass_description[0].inputAttachmentCount    = 0;
    subpass_description[0].pInputAttachments       = nullptr;
    subpass_description[0].preserveAttachmentCount = 0;
    subpass_description[0].pPreserveAttachments    = nullptr;
    subpass_description[0].pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::vector<VkSubpassDependency> dependencies(2);

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    m_swap_chain_render_pass = RenderPass::create(backend, attachments, subpass_description, dependencies);
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