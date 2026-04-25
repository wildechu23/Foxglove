#pragma once

#include "foxglove/core/handle.h"
#include "foxglove/vulkan/vulkan_context.h"
#include "foxglove/resources/desc.h"

#include <string>

using BufferHandle = TaggedHandle<ResourceType::Buffer>;
using TextureHandle = TaggedHandle<ResourceType::Texture>;

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
        vmaCreateBuffer(allocator, &info, &allocInfo, 
                       &buffer, &allocation, &allocInfoDetail);
    
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

    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent2D extent;
    
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

        VkImageSubresourceRange rview_srr = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .layerCount = 1
        };


        VkImageViewCreateInfo rview_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = desc.format,
            .subresourceRange = rview_srr
        };

        vkCreateImageView(
                device, 
                &rview_info, 
                nullptr, 
                &view);

        extent = desc.extent;
    }
    
    void destroy(VkDevice device, VmaAllocator allocator) {
        // TODO: add check that its initialized
        vkDestroyImageView(device, view, nullptr);
        vmaDestroyImage(allocator, image, allocation);
    }
};
