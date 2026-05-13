#include "foxglove/resources/resource_manager.h"

ResourceManager::ResourceManager(VulkanContext& ctx) {
    m_device = ctx.get_device();
    m_allocator = ctx.get_allocator();
}

void ResourceManager::init() {}

void ResourceManager::cleanup() {
    m_buffers.cleanup(m_allocator);
    m_textures.cleanup(m_device, m_allocator);
    m_samplers.cleanup(m_device);
}

BufferHandle ResourceManager::create_buffer(const BufferDesc& desc) {
    return m_buffers.create(m_device, m_allocator, desc);
}

TextureHandle ResourceManager::create_texture(const TextureDesc& desc) {
    return m_textures.create(m_device, m_allocator, desc);
}

SamplerHandle ResourceManager::create_sampler(const SamplerDesc& desc) {
    return m_samplers.create(m_device, desc);
}
    
BufferResource* ResourceManager::get_buffer(BufferHandle handle) {
    return m_buffers.get(handle);
}

TextureResource* ResourceManager::get_texture(TextureHandle handle) {
    return m_textures.get(handle);
}

SamplerResource* ResourceManager::get_sampler(SamplerHandle handle) {
    return m_samplers.get(handle);
}

VkDeviceAddress ResourceManager::get_buffer_address(BufferHandle handle) {
    BufferResource* resource = get_buffer(handle);
    return _get_buffer_address(resource);
}

void ResourceManager::destroy_buffer(BufferHandle handle) {
    m_buffers.destroy(handle, m_allocator);
}

void ResourceManager::destroy_texture(TextureHandle handle) {
    m_textures.destroy(handle, m_device, m_allocator);
}

void ResourceManager::destroy_sampler(SamplerHandle handle) {
    m_samplers.destroy(handle, m_device);
}


VkDeviceAddress ResourceManager::_get_buffer_address(BufferResource* resource) {
    VkBufferDeviceAddressInfo device_address_info = { 
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = resource->buffer
	};
    return vkGetBufferDeviceAddress(m_device, &device_address_info);
}
