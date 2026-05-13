#pragma once

#include "foxglove/resources/resource.h"
#include "foxglove/resources/resource_pool.h"

// MAKE THIS FOR PERMANENT RESOURCES
class ResourceManager {
public:
    ResourceManager(VulkanContext& ctx);
    ~ResourceManager() = default;
    
    void init();
    void cleanup();

    BufferHandle create_buffer(const BufferDesc& desc);
    TextureHandle create_texture(const TextureDesc& desc);
    SamplerHandle create_sampler(const SamplerDesc& desc);

    BufferResource* get_buffer(BufferHandle handle);
    TextureResource* get_texture(TextureHandle handle);
    SamplerResource* get_sampler(SamplerHandle handle);

    VkDeviceAddress get_buffer_address(BufferHandle handle);
   
    void destroy_buffer(BufferHandle handle);
    void destroy_texture(TextureHandle handle);
    void destroy_sampler(SamplerHandle handle);
private:
    VkDeviceAddress _get_buffer_address(BufferResource* resource);

    VkDevice m_device;
    VmaAllocator m_allocator;

    BufferPool m_buffers;
    TexturePool m_textures;
    SamplerPool m_samplers;

    friend class DescriptorHeapAllocator;
};

