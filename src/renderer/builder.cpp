#include "foxglove/renderer/builder.h"
#include "foxglove/renderer/framegraph.h"

#include <iostream>

//
// PassBuilder
//

PassBuilder::PassBuilder(FrameGraph* fg, const std::string& name, PassType type) : m_fg(fg), m_name(name), m_type(type) {
    m_render_area = VkRect2D {
        { 0, 0 },
        fg->m_swapchain->get_extent()
    };
    m_layer_count = 1;
}

void PassBuilder::check_init(uint32_t set) {
    if(m_bind_groups.find(set) == m_bind_groups.end()) {
        m_bind_groups[set] = { 
            .set = set, 
            .buffers = {}, 
            .textures = {} 
        };
    }
}

// TODO: ADD HANDLE 
PassBuilder& PassBuilder::bind_buffer(FGBufferHandle handle,
        BufferUsage usage, ResourceAccess access,
        uint32_t set, uint32_t binding) {
    check_init(set);

    m_bind_groups[set].buffers.push_back({
        handle, usage, access, binding
    });
    return *this;
}

PassBuilder& PassBuilder::bind_texture(FGTextureHandle handle,
        TextureUsage usage, ResourceAccess access,
        uint32_t set, uint32_t binding) {
    check_init(set);

    m_bind_groups[set].textures.push_back({
        handle, usage, access, binding
    });
    return *this;
}

PassBuilder& PassBuilder::bind_color_attachment(
        FGTextureHandle handle, 
        LoadOp load_op, StoreOp store_op,
        std::optional<Color> clear_value) {
    m_color_attachments.push_back({
        handle, load_op, store_op, clear_value
    });
    return *this;
}

PassBuilder& PassBuilder::bind_depth_attachment(
        FGTextureHandle handle, 
        LoadOp load_op, StoreOp store_op,
        std::optional<float> clear_value) {
    m_depth_attachment = {
        handle, load_op, store_op, clear_value
    };
    return *this;
}

PassBuilder& PassBuilder::bind_stencil_attachment(
        FGTextureHandle handle, 
        LoadOp load_op, StoreOp store_op,
        std::optional<uint32_t> clear_value) {
    m_stencil_attachment = {
        handle, load_op, store_op, clear_value
    };
    return *this;
}

PassBuilder& PassBuilder::bind_depth_stencil_attachment(
        FGTextureHandle handle, 
        LoadOp load_op, StoreOp store_op,
        std::optional<float> depth_clear_value,
        std::optional<uint32_t> stencil_clear_value) {
    bind_depth_attachment(handle, load_op, store_op, depth_clear_value);
    bind_stencil_attachment(handle, load_op, store_op, stencil_clear_value);
    return *this;
}

PassBuilder& PassBuilder::present(FGTextureHandle handle) {
    assert(m_type == PassType::Present);
    
    // does need to be transfer src
    // should this even be used? it will fracture transitions into two commands
    bind_texture(
            handle, 
            TextureUsage::TransferSrc, 
            ResourceAccess::Read,
            0, 0 // 0, 0 here is useless consider fixing
    );
    
    // in m_execute_fn
    m_execute_fn = [this, handle](PassContext& ctx) {
        VkCommandBuffer cmd = ctx.get_cmd();
        
        FrameGraph* fg = this->m_fg;
        //Swapchain* swapchain = fg->m_swapchain;
        
        // BOTH THESE HANDLES ARE INVALID
        FGTexture* texture = fg->m_textures.get(handle);
        FGTexture* swapchain = fg->m_textures.get(
                ctx.get_swapchain_handle());
        
        // copy image
        VkImage& src_image = texture->get_resource()->image;
        VkExtent2D src_extent = texture->get_resource()->extent;

        VkImage& swapchain_image = swapchain->get_resource()->image;
        VkExtent2D swapchain_extent = swapchain->get_resource()->extent;
        
        // should i use handles?
        util::copy_image_to_image(cmd, 
                src_image, swapchain_image,
                src_extent, swapchain_extent);

        // swap
        util::transition_image_to_present(cmd, swapchain_image);
    };

    return *this;
}

PassBuilder& PassBuilder::clear_color(FGTextureHandle handle, Color color) {
    assert(m_type == PassType::Clear);

    bind_texture(
            handle, 
            TextureUsage::StorageImage, 
            ResourceAccess::Write,
            0, 0 // 0, 0 here is useless consider fixing
    );
    
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
        VkImage image = texture->get_resource()->image;
        vkCmdClearColorImage(ctx.get_cmd(), image, 
                VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);
    });
    return *this;
}


PassBuilder& PassBuilder::execute(std::function<void(PassContext&)> execute_fn) {
    m_execute_fn = execute_fn;
    return *this;
}

Pass* PassBuilder::build() {
    // validate
    // TODO: VERIFY THE BELOW CONSTRUCT IS CORRECT
    
    // Note bindings are stored in user-declared order
    std::vector<BindingGroup> binding_groups;
    binding_groups.reserve(m_bind_groups.size());
    
    // random order (is ordered faster)
    for(const auto& [set, group] : m_bind_groups) {
        binding_groups.push_back(group);
    }


    PassDesc desc = PassDesc(m_name, m_type,
                std::move(binding_groups),
                std::move(m_execute_fn));

    std::unique_ptr<Pass> pass;
    switch(m_type) { 
        case(PassType::Compute): {
            pass = std::make_unique<ComputePass>(desc);
            break;
        }
        case(PassType::Graphics): {
            GraphicsPassInfo info = {
                m_color_attachments,
                m_depth_attachment,
                m_stencil_attachment,
                m_render_area,
                m_layer_count
            };
            
            pass = std::make_unique<GraphicsPass>(desc, info);
            break;
        }
        default: {
            pass = std::make_unique<Pass>(desc);
            break;
        }
    }

    Pass* pass_ref = pass.get();
    m_fg->add_pass(std::move(pass));
    return pass_ref;
}


