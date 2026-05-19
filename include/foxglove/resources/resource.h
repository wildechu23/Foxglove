#pragma once

#include "foxglove/core/handle.h"
#include "foxglove/vulkan/vulkan_context.h"
#include "foxglove/resources/desc.h"

#include <string>
#include <iostream>

using BufferHandle = TaggedHandle<ResourceType::Buffer>;
using TextureHandle = TaggedHandle<ResourceType::Texture>;
using SamplerHandle = TaggedHandle<ResourceType::Sampler>;

enum ResourceAccess {
    None = 0,
    Read = 1,
    Write = 2,
    ReadWrite = Read | Write
};

enum class BufferUsage {
    Unknown,
    StorageBuffer,
    UniformBuffer,
    VertexBuffer,
    IndexBuffer,
    TransferSrc,
    TransferDst
    // Accleration structure
};

enum class TextureUsage {
    Unknown,
    ColorAttachment,
    DepthAttachment,
    StencilAttachment,
    DepthStencilAttachment,
    InputAttachment,
    SampledImage,
    StorageImage,
    TransferSrc,
    TransferDst
};

// Resources
// TODO: MOVE OUT IMPLEMENTATION?
// MOVE TO VULKANBUFFERRESOURCE, KEEP GENERIC VERSION
class BufferResource {
public:
    BufferResource() = default;
    BufferResource(VkDevice device, VmaAllocator allocator,
            const BufferDesc& desc) {
        create(device, allocator, desc);
    }

    VkBuffer buffer;
    VmaAllocation allocation;
    size_t size;
    void* mapped_data = nullptr;


    // state
    BufferUsage last_usage;
    ResourceAccess last_access;
    
    void create(VkDevice device, VmaAllocator allocator, 
                const BufferDesc& desc) {
        VkBufferCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,	
            .size = desc.size,
            .usage = desc.usage
        };

        VmaAllocationCreateInfo allocInfo {
	        .flags = desc.allocation_flags,
            .usage = desc.memory_usage
        };        

        VmaAllocationInfo allocInfoDetail;
        VkResult result = vmaCreateBuffer(allocator, &info, &allocInfo, 
                       &buffer, &allocation, &allocInfoDetail);
        if(result != VK_SUCCESS) {
            std::cerr << "buffer creation failed" << std::endl;
        }

    
        // assign member vars
        size = info.size;
        
        if(desc.allocation_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
            mapped_data = allocInfoDetail.pMappedData;
        }
    }
    
    void destroy(VmaAllocator allocator) {
        if (buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, buffer, allocation);
            buffer = VK_NULL_HANDLE;
        }
    }
};

class TextureResource {
public:
    TextureResource() = default;
    TextureResource(VkDevice device, VmaAllocator allocator,
            const TextureDesc& desc) {
        create(device, allocator, desc);
    }
    
    // TODO: CONSIDER ADDING ACCESS AND USAGE HERE TO AVOID
    // MORE TRANSITIONS
    VkImage image;
    VkImageView view = VK_NULL_HANDLE;
    VmaAllocation allocation;

    // desc
    VkExtent2D extent;
    VkFormat format;
    VkImageUsageFlags usage;
    uint32_t mip_levels;
    uint32_t array_layers;


    // state
    TextureUsage last_usage;
    ResourceAccess last_access;
    
    void create(VkDevice device, VmaAllocator allocator, 
                const TextureDesc& desc) {
        VkImageCreateInfo rimg_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = desc.format,
            .extent = {desc.extent.width, desc.extent.height, 1},
            .mipLevels = desc.mip_levels,
            .arrayLayers = desc.array_layers,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = desc.usage
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
                &image, 
                &allocation,
                nullptr);

        extent = desc.extent;
        format = desc.format;
        usage = desc.usage;
        mip_levels = desc.mip_levels;
        array_layers = desc.array_layers;

        if(desc.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT 
                || desc.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            VkImageViewCreateInfo info = get_view_create_info();
            VkResult result = vkCreateImageView(device, &info, nullptr, &view);
            if(result != VK_SUCCESS) {
                std::cerr << "Failed to create image view" << std::endl;
            }
        }
    }
    
    void destroy(VkDevice device, VmaAllocator allocator) {
        if(view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, view, nullptr);
        }
        vmaDestroyImage(allocator, image, allocation);
    }

    VkImageViewCreateInfo get_view_create_info() {
        VkImageSubresourceRange rview_srr = {
            .aspectMask = get_aspect_mask(usage, format),
            .levelCount = mip_levels, 
            .layerCount = array_layers
        };


        VkImageViewType view_type = (array_layers > 1) 
            ? VK_IMAGE_VIEW_TYPE_2D_ARRAY 
            : VK_IMAGE_VIEW_TYPE_2D;


        return VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .image = image,
            .viewType = view_type,
            .format = format,
            .subresourceRange = rview_srr
        };
    }



    VkImageAspectFlags get_aspect_mask(VkImageUsageFlags usage,
            VkFormat format) {
        VkImageAspectFlags aspect_mask = 0;

        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            switch (format) {
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_X8_D24_UNORM_PACK32:
                case VK_FORMAT_D32_SFLOAT:
                    aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    break;

                case VK_FORMAT_S8_UINT:
                    aspect_mask = VK_IMAGE_ASPECT_STENCIL_BIT;
                    break;

                case VK_FORMAT_D16_UNORM_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                    aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT 
                        | VK_IMAGE_ASPECT_STENCIL_BIT;
                    break;

                default:
                    assert(false && "Format doesn't support depth/stencil");
                    aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    break;
            }
        } else {
            aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        return aspect_mask;
    }
};


class SamplerResource {
public:
    SamplerResource() = default;
    SamplerResource(VkDevice device, const SamplerDesc& desc) {
        create(device, desc);
    }

    VkSampler sampler;
    VkSamplerCreateInfo info;

    void create(VkDevice device, const SamplerDesc& desc) {
        info = { 
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .magFilter = desc.mag_filter,
            .minFilter = desc.min_filter,
            .mipmapMode = desc.mipmap_mode,
            .minLod = desc.min_lod,
            .maxLod = desc.max_lod,
            .unnormalizedCoordinates = VK_FALSE
        };

        vkCreateSampler(device, &info, nullptr, &sampler);
    }

    void destroy(VkDevice device) {
        vkDestroySampler(device, sampler, nullptr);
    }
};
