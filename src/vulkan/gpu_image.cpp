#include "foxglove/vulkan/gpu_image.h"

GPUImage::GPUImage() {};
GPUImage::~GPUImage() {};

void GPUImage::create(VkDevice device, VmaAllocator allocator,
        VkFormat format, VkExtent3D ext,
        VkImageUsageFlags usage) {
    // set format and extent
    m_format = format;
    m_extent = {ext.width, ext.height};

    VkImageCreateInfo rimg_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R16G16B16A16_SFLOAT,
		.extent = ext,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage
    };

    VmaAllocationCreateInfo rimg_allocinfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    

    vmaCreateImage(
            allocator,
            &rimg_info,
            &rimg_allocinfo,
            &m_image, 
            &m_allocation,
            nullptr);

    VkImageSubresourceRange rview_srr = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .layerCount = 1
    };

    VkImageViewCreateInfo rview_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .image = m_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = m_format,
        .subresourceRange = rview_srr
    };

    vkCreateImageView(
            device, 
            &rview_info, 
            nullptr, 
            &m_image_view);
}

void GPUImage::cleanup(VkDevice device, VmaAllocator allocator) {
    vkDestroyImageView(device, m_image_view, nullptr);
    vmaDestroyImage(allocator, m_image, m_allocation);
}

