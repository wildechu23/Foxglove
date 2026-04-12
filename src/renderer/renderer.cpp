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
    m_sl.init(m_ctx.get_device());
    m_pm.init(m_ctx.get_device());
    
    // initialize draw images
    /*VkExtent3D draw_image_extent = {
        m_window.get_width(),
        m_window.get_height(),
        1
    };*/

    //m_frames = std::vector<FrameContext>(m_swapchain.get_image_count());

    for(uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        m_frames[i].init(m_ctx);
    }
}


void Renderer::draw() {
    VkDevice device = m_ctx.get_device();

    FrameContext& fctx = get_current_frame();
    vkWaitForFences(device, 1, &fctx.get_render_fence(), true, UINT64_MAX);
    vkResetFences(device, 1, &fctx.get_render_fence());

	fctx.get_deletion_queue().flush();
    
    uint32_t swapchain_image_idx;
    vkAcquireNextImageKHR(
            device,
            m_swapchain.get_swapchain(), 
            1000000000, 
            fctx.get_swapchain_semaphore(), 
            nullptr, 
            &swapchain_image_idx);

    VkSemaphore render_semaphore = m_swapchain.get_render_semaphore(
            swapchain_image_idx);
    
    // register swapchain image
    TextureDesc swapchain_image_desc = {
        .extent = m_swapchain.get_extent(),
        .format = m_swapchain.get_format(),
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
    };

    TextureResource* swap_ptr = new TextureResource;
    swap_ptr->image = m_swapchain.get_image(swapchain_image_idx);
    swap_ptr->view = m_swapchain.get_image_view(swapchain_image_idx);
    swap_ptr->extent = m_swapchain.get_extent();

    FGTextureHandle swapchain_handle = m_fg.register_external_texture(
            "swapchain", swapchain_image_desc, swap_ptr);
    fctx.set_swapchain_handle(swapchain_handle);
    
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
		.semaphore = render_semaphore, 
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
		.pWaitSemaphores = &render_semaphore,
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
    
    m_fg.reset();

    m_pm.cleanup();
    m_sl.cleanup();
    
    m_swapchain.cleanup();
    m_ctx.cleanup();
}
