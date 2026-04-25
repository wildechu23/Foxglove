#pragma once

#include "foxglove/resources/resource_pool.h"
#include "foxglove/resources/handle.h"

/*
// TODO: CONSIDER ADDING CREATE AND DESTROY TO BASE CLASS?
class TexturePool : public ResourcePool<TextureResource, TextureHandle> {
    VkDevice device;
    VmaAllocator allocator;

public:
    TexturePool(VkDevice dev, VmaAllocator alloc) 
        : device(dev), allocator(alloc) {}

    TextureHandle create_texture(const TextureDesc& desc) {
        TextureHandle handle = create();
        if (TextureResource* resource = get(handle)) {
            resource->create(device, allocator, desc);
        }
        return handle;
    }

    void destroy_texture(TextureHandle handle) {
        if (TextureResource* resource = get(handle)) {
            resource->destroy(device, allocator);
        }
        destroy(handle);
    }
};

*/
