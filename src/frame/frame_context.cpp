#include "foxglove/frame/frame_context.h"

#include <cstdio>
FrameContext::FrameContext() {}
FrameContext::~FrameContext() {}

void FrameContext::init(VulkanContext& ctx, VkExtent3D ext) {
    VkDevice device = ctx.get_device();

    // init commands
    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx.get_graphics_queue_family()
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
    vkCreateSemaphore(device, &semaphore_create_info, nullptr, &m_render_semaphore);

    // TODO: REMOVE DRAW IMAGE
    // init draw image
    VkImageUsageFlags draw_image_usages = VkImageUsageFlags(
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
            );

    m_draw_image.create(
            device,
            ctx.get_allocator(),
            VK_FORMAT_R16G16B16A16_SFLOAT,
            ext,
            draw_image_usages);
}

void FrameContext::cleanup(VkDevice device, VmaAllocator allocator) {
    m_draw_image.cleanup(device, allocator);

    vkDestroyCommandPool(
            device,
            m_cmd_pool,
            nullptr);

    vkDestroyFence(device, m_render_fence, nullptr);
    vkDestroySemaphore(device, m_render_semaphore, nullptr);
    vkDestroySemaphore(device, m_swapchain_semaphore, nullptr);
}
