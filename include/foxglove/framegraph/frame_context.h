#pragma once

#include "foxglove/vulkan/vulkan_context.h"
#include "foxglove/vulkan/descriptors.h"

#include "foxglove/resources/pipeline/pipeline.h"

#include "foxglove/framegraph/fg_resource.h"

#include <vulkan/vulkan.h>
#include <queue>
#include <functional>

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		for(auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)();
		}
		deletors.clear();
	}
};

class FrameContext {
public:
    FrameContext() = default;
    ~FrameContext() = default;

    void init(VulkanContext* ctx);
    void cleanup();

    VkFence& get_render_fence() { return m_render_fence; }
    VkSemaphore& get_swapchain_semaphore() { return m_swapchain_semaphore; }
    //VkSemaphore& get_render_semaphore() { return m_render_semaphore; }

    VkCommandBuffer get_cmd_buffer() { return m_cmd_buffer; }
    const FGTextureHandle& get_swapchain_handle() { return m_swapchain_handle; }
    void set_swapchain_handle(FGTextureHandle handle) { m_swapchain_handle = handle; }
    
    DescriptorAllocator& get_descriptor_allocator() {
        return m_descriptor_allocator;
    }
    
    VkDevice get_device() const { return m_device; }
    DeletionQueue& get_deletion_queue() { return m_deletion_queue; }

private:
    VkDevice m_device;

	VkCommandPool m_cmd_pool;
	VkCommandBuffer m_cmd_buffer;
	
	VkSemaphore m_swapchain_semaphore;
	VkFence m_render_fence;
    
    FGTextureHandle m_swapchain_handle;
    
    DescriptorAllocator m_descriptor_allocator;

    DeletionQueue m_deletion_queue;
};
