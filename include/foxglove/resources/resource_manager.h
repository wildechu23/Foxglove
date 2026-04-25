#pragma once

#include "foxglove/resources/resource.h"
#include "foxglove/resources/resource_pool.h"

// MAKE THIS FOR PERMANENT RESOURCES
class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    void init(VulkanContext* ctx);
    void cleanup();

    BufferHandle create_buffer(const BufferDesc& desc);
    TextureHandle create_texture(const TextureDesc& desc);

    BufferResource* get_buffer(BufferHandle handle);
    TextureResource* get_texture(TextureHandle handle);

    VkDeviceAddress get_buffer_address(BufferHandle handle);
   
    void destroy_buffer(BufferHandle handle);
    void destroy_texture(TextureHandle handle);
private:
    VkDevice m_device;
    VmaAllocator m_allocator;

    BufferPool m_buffers;
    TexturePool m_textures;
};

