#include "foxglove/renderer/renderer.h"

Renderer::Renderer(Window& window)
    : m_window(window) {
    init();
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::init() {
    m_ctx.init(m_window.get_window());

    m_swapchain.init(
        m_ctx.get_device(),
        m_ctx.get_physical_device(),
        m_ctx.get_surface(),
        m_window.get_width(),
        m_window.get_height()
    );

    m_fg.init(&m_ctx, &m_swapchain);
    
    printf("draw images\n");

    // initialize draw images
    VkExtent3D draw_image_extent = {
        m_window.get_width(),
        m_window.get_height(),
        1
    };

    for(uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        m_frames[i].init(m_ctx, draw_image_extent);
    }
}

void Renderer::draw() {
    VkDevice device = m_ctx.get_device();

    FrameContext& fctx = get_current_frame();

    vkWaitForFences(device, 1, &fctx.get_render_fence(), true, UINT64_MAX);
    vkResetFences(device, 1, &fctx.get_render_fence());

    uint32_t swapchain_image_idx;
    vkAcquireNextImageKHR(
            device,
            m_swapchain.get_swapchain(), 
            1000000000, 
            fctx.get_swapchain_semaphore(), 
            nullptr, 
            &swapchain_image_idx);
    
    // TODO: ADDED update framecontext (framecontext has no setters besides this)
    fctx.set_swapchain_idx(swapchain_image_idx);

	VkCommandBuffer cmd = fctx.get_cmd_buffer();
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo cmd_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

    // start of command buffer
	vkBeginCommandBuffer(cmd, &cmd_begin_info);

    m_fg.execute(fctx);
	
    //utils::transition_image(cmd, draw_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // add draws here
    
   // utils::transition_image(cmd, draw_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    //utils::transition_image(cmd, swapchain_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    //utils::copy_image_to_image(cmd, draw_image, swapchain_image, draw_image_extent, swapchain_extent);

    //utils::transition_image(cmd, m_swapchain.get_image(swapchain_image_idx), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    

	vkEndCommandBuffer(cmd);

    // end of command buffer

	VkCommandBufferSubmitInfo cmd_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = cmd,
		.deviceMask = 0
	};

	VkSemaphoreSubmitInfo wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = fctx.get_swapchain_semaphore(),
		.value = 1,
		.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
		.deviceIndex = 0,
	};

	VkSemaphoreSubmitInfo signal_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = fctx.get_render_semaphore(),
		.value = 1,
		.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
		.deviceIndex = 0
	};

	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.waitSemaphoreInfoCount = 1,
		.pWaitSemaphoreInfos = &wait_info,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &cmd_info,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signal_info
	};
   
    VkQueue& graphics_queue = m_ctx.get_graphics_queue();
    vkQueueSubmit2(graphics_queue, 1, &submit, fctx.get_render_fence());

	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &fctx.get_render_semaphore(),
		.swapchainCount = 1,
		.pSwapchains = &m_swapchain.get_swapchain(),
		.pImageIndices = &swapchain_image_idx,
	};

    vkQueuePresentKHR(graphics_queue, &present_info);
    
    m_frame_num++;

}

void Renderer::cleanup() {
    vkDeviceWaitIdle(m_ctx.get_device());

    for(uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        m_frames[i].cleanup(m_ctx.get_device(), m_ctx.get_allocator());
    }

    m_swapchain.cleanup();
    m_ctx.cleanup();
}
