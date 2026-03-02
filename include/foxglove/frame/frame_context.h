#pragma once

#include "foxglove/vulkan/vulkan_context.h"
#include "foxglove/vulkan/gpu_image.h"

#include <vulkan/vulkan.h>

struct PassContext {
    VkCommandBuffer cmd;
    uint32_t swapchain_idx;
};

class FrameContext {
public:
    FrameContext();
    ~FrameContext();

    void init(VulkanContext& ctx, VkExtent3D ext);
    void cleanup(VkDevice device, VmaAllocator allocator);

    VkFence& get_render_fence() { return m_render_fence; }
    VkSemaphore& get_swapchain_semaphore() { return m_swapchain_semaphore; }
    VkSemaphore& get_render_semaphore() { return m_render_semaphore; }

    VkCommandBuffer get_cmd_buffer() { return m_cmd_buffer; }
    GPUImage& get_image() { return m_draw_image; }
    
    const uint32_t& get_swapchain_idx() { return m_swapchain_idx; }
    void set_swapchain_idx(uint32_t idx) { m_swapchain_idx = idx; }
    
    PassContext pass_view() const {
        return PassContext {
            m_cmd_buffer,
            m_swapchain_idx,
        };
    }
private:
	VkCommandPool m_cmd_pool;
	VkCommandBuffer m_cmd_buffer;
	
	VkSemaphore m_swapchain_semaphore, m_render_semaphore;
	VkFence m_render_fence;

    GPUImage m_draw_image;
    
    uint32_t m_swapchain_idx;
};
