#pragma once

#include "foxglove/resources/resource_pool.h"
#include "foxglove/core/handle.h"

/*
class BufferPool : public ResourcePool<BufferResource, BufferHandle> {
    VkDevice device;
    VmaAllocator allocator;

public:
    BufferPool(VkDevice dev, VmaAllocator alloc) 
        : device(dev), allocator(alloc) {}

    BufferHandle create_buffer(const BufferDesc& desc) {
        BufferHandle handle = create();
        if (BufferResource* resource = get(handle)) {
            resource->create(device, allocator, desc);
        }
        return handle;
    }

    void destroy_buffer(BufferHandle handle) {
        if (BufferResource* resource = get(handle)) {
            resource->destroy(allocator);
        }
        destroy(handle);
    }

    // FREE TO ADD BUFFER SPECIFIC FUNCTIONS
};
*/
