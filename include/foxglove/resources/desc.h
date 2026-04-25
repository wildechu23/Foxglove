#pragma once

#include "foxglove/vulkan/vulkan_context.h"

// TODO: ADD MIP AND ARRAY
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
