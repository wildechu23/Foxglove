#pragma once

#include "foxglove/vulkan/vulkan_context.h"

struct TextureDesc {
    VkExtent2D extent;
    VkFormat format;
    VkImageUsageFlags usage;
    uint32_t mip_levels = 1;
    uint32_t array_layers = 1;
    // ... other properties
};

struct BufferDesc {
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VmaMemoryUsage memory_usage;    
    VmaAllocationCreateFlags allocation_flags = 0;
    // ... other properties
};

// TODO: add other properties of VkSamplerCreateInfo
struct SamplerDesc {
    VkFilter mag_filter;
    VkFilter min_filter;
    VkSamplerMipmapMode mipmap_mode;
    float min_lod;
    float max_lod;
};
