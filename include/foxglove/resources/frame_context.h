#pragma once

#include "foxglove/vulkan/vulkan_context.h"
#include "foxglove/vulkan/gpu_image.h"

#include "foxglove/resources/handle.h"

#include <vulkan/vulkan.h>

struct PassContext {
    VkCommandBuffer cmd;
    FGTextureHandle swapchain_handle;
};

class FrameContext {
public:
    FrameContext();
    ~FrameContext();

    void init(VulkanContext& ctx);
    void cleanup(VkDevice device, VmaAllocator allocator);

    VkFence& get_render_fence() { return m_render_fence; }
    VkSemaphore& get_swapchain_semaphore() { return m_swapchain_semaphore; }
    VkSemaphore& get_render_semaphore() { return m_render_semaphore; }

    VkCommandBuffer get_cmd_buffer() { return m_cmd_buffer; }
    
    const FGTextureHandle& get_swapchain_handle() { return m_swapchain_handle; }
    void set_swapchain_handle(FGTextureHandle handle) { m_swapchain_handle = handle; }
    
    PassContext pass_view() const {
        return PassContext {
            m_cmd_buffer,
            m_swapchain_handle,
        };
    }
private:
	VkCommandPool m_cmd_pool;
	VkCommandBuffer m_cmd_buffer;
	
	VkSemaphore m_swapchain_semaphore, m_render_semaphore;
	VkFence m_render_fence;
    
    FGTextureHandle m_swapchain_handle;
};
