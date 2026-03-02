#pragma once

#include "foxglove/resources/resource_types.h"
#include "foxglove/resources/handle_registry.h"

/*
// MAKE THIS FOR PERMANENT RESOURCES
class ResourceManager {
public:
    ResourceManager(VkDevice device, VmaAllocator allocator)
        : m_buffers(device, allocator),
          m_textures(device, allocator) {}
    //~ResourceManager() {};
    //void init(VulkanContext& ctx);
    //void cleanup();

    BufferHandle create_buffer(const BufferDesc& desc);
    TextureHandle create_texture(const TextureDesc& desc);

    BufferResource* get_buffer(BufferHandle handle);
    TextureResource* get_texture(TextureHandle handle);
    
    void destroy_buffer(BufferHandle handle);
    void destroy_texture(TextureHandle handle);
private:
    //VulkanContext& m_ctx;
    //BufferPool m_buffers; 
    //TexturePool m_textures;
    
    //FGBufferRegistry m_buffers;
    //FGTextureRegistry m_textures;
};
*/

