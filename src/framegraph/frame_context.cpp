#include "foxglove/framegraph/frame_context.h"

#include <cstdio>

void FrameContext::init(VulkanContext* ctx, DescriptorHeapAllocator* heap) {
    VkDevice device = ctx->get_device();
    
    // TODO: PROBABLY INITIALIZE MORE
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	};

	m_descriptor_allocator.init_pool(device, 10, sizes);

    // init commands
    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx->get_graphics_queue_family()
    };

    vkCreateCommandPool(
            device,
            &cmd_pool_info, 
            nullptr,
            &m_cmd_pool);

    VkCommandBufferAllocateInfo cmd_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = m_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(
            device,
            &cmd_alloc_info,
            &m_cmd_buffer);

    // init sync
    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr
    };

    vkCreateFence(device, &fence_create_info, nullptr, &m_render_fence);
    vkCreateSemaphore(device, &semaphore_create_info, nullptr, &m_swapchain_semaphore);

    m_device = device;
    m_descriptor_heap = heap;
}

void FrameContext::cleanup() {
    vkDestroyCommandPool(
            m_device,
            m_cmd_pool,
            nullptr);

    vkDestroyFence(m_device, m_render_fence, nullptr);
    vkDestroySemaphore(m_device, m_swapchain_semaphore, nullptr);

    m_deletion_queue.flush();

	m_descriptor_allocator.destroy_pool(m_device);
}
