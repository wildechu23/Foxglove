#include "foxglove/renderer/framegraph_pass.h"
#include "foxglove/renderer/framegraph.h"

#include <iostream>
//
// PassBuilder
//

// TODO: ADD HANDLE 
PassBuilder& PassBuilder::bind_buffer(FGBufferHandle handle,
        BufferUsage usage, ResourceAccess access) {
    m_buffers.push_back(BufferBinding{
            handle,
            usage,
            access
    });
    return *this;
}

PassBuilder& PassBuilder::bind_texture(FGTextureHandle handle,
        TextureUsage usage, ResourceAccess access) {
    m_textures.push_back(TextureBinding{
            handle,
            usage,
            access
    });

    return *this;
}

PassBuilder& PassBuilder::present(FGTextureHandle handle) {
    assert(m_type == PassType::Present);
    
    // does need to be transfer src
    // should this even be used? it will fracture transitions into two commands
    m_textures.push_back(TextureBinding{
            handle,
            TextureUsage::TransferSrc,
            ResourceAccess::Read
    });

    
    // in m_execute_fn
    m_execute_fn = [this, handle](PassContext ctx) {
        FrameGraph* fg = this->m_fg;
        //Swapchain* swapchain = fg->m_swapchain;
        
        // BOTH THESE HANDLES ARE INVALID
        FGTexture* texture = fg->m_textures.get(handle);
        FGTexture* swapchain = fg->m_textures.get(ctx.swapchain_handle);
        
            
        // copy image
        VkImage& src_image = texture->get_resource()->image;
        VkExtent2D src_extent = texture->get_resource()->extent;

        VkImage& swapchain_image = swapchain->get_resource()->image;
        VkExtent2D swapchain_extent = swapchain->get_resource()->extent;
        
        // should i use handles?
        util::copy_image_to_image(ctx.cmd, src_image, swapchain_image,
                src_extent, swapchain_extent);

        // swap
        util::transition_image_to_present(ctx.cmd, swapchain_image);
    };

    return *this;
}

PassBuilder& PassBuilder::clear_color(FGTextureHandle handle, Color color) {
    
    execute([this, handle, color](PassContext ctx) {
        FGTexture* texture = this->m_fg->m_textures.get(handle);
        VkClearColorValue clear_value = {
            color.r, color.g, color.b, color.a
        };

        VkImageSubresourceRange clear_range = {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, VK_REMAINING_MIP_LEVELS,
            0, VK_REMAINING_ARRAY_LAYERS
        };
        // storage_image -> general?
        vkCmdClearColorImage(ctx.cmd, texture->get_resource()->image,
                VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);
    });
    return *this;
}


PassBuilder& PassBuilder::execute(std::function<void(PassContext)> execute_fn) {
    m_execute_fn = execute_fn;
    return *this;
}

Pass& PassBuilder::build() {
    // validate
    // TODO: VERIFY THE BELOW CONSTRUCT IS CORRECT
    std::unique_ptr<Pass> pass = std::make_unique<Pass>(PassDesc(
        m_name,
        m_type,
        std::move(m_buffers),
        std::move(m_textures),
        std::move(m_execute_fn)
    ));
    
    Pass& pass_ref = *pass;
    m_fg->add_pass(std::move(pass));
    return pass_ref;
}


