#include "foxglove/resources/pass_context.h"

void PassContext::bind_compute_pipeline(ComputePipeline* pipeline) {
    assert(m_pass->get_type() == PassType::Compute);
    // bind pipeline
	vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, 
            pipeline->get_pipeline());

    // build writes
    DescriptorAllocator* desc_allocator = &m_fctx->get_descriptor_allocator();
    std::vector<VkDescriptorSet> desc_sets = desc_allocator->allocate(
            m_device, pipeline->m_descriptor_layouts);
    const std::vector<BindingGroup>& bind_groups = m_pass->get_bind_groups();

    std::vector<VkWriteDescriptorSet> write_sets;
    std::vector<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkDescriptorImageInfo> image_infos;

    int buffer_size = 0;
    int image_size = 0;
    for(const BindingGroup& g : bind_groups) {
        buffer_size += g.buffers.size();
        image_size += g.textures.size();
    }

    buffer_infos.reserve(buffer_size);
    image_infos.reserve(image_size);

    for(const BindingGroup& g : bind_groups) {
        VkDescriptorSet& dst_set = desc_sets[g.set];
       
        for(const BufferBinding& bb : g.buffers) {
            FGBuffer* fg_buffer = m_buffers.get(bb.handle);
            BufferResource* buffer = fg_buffer->get_resource();

            VkDescriptorBufferInfo buffer_info = {
                .buffer = buffer->buffer, 
                .offset = 0, 
                .range = buffer->size
            };
            buffer_infos.push_back(buffer_info);

            // TODO: binding.binding??
            write_sets.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = dst_set,
                .dstBinding = bb.binding,
                .descriptorCount = 1,
                .descriptorType = util::deduce_descriptor_type(bb.usage),
                .pBufferInfo = &buffer_infos.back() 
            });
        }

        for(const TextureBinding& tb : g.textures) {
            FGTexture* fg_texture = m_textures.get(tb.handle);
            TextureResource* texture = fg_texture->get_resource();

            // sampler goes here too
            VkDescriptorImageInfo image_info = {
                .imageView = texture->view,
                .imageLayout = util::deduce_layout(tb.usage)
            };
            image_infos.push_back(image_info);

            write_sets.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = dst_set,
                .dstBinding = tb.binding,
                .descriptorCount = 1,
                .descriptorType = util::deduce_descriptor_type(tb.usage),
                .pImageInfo = &image_infos.back() 
            });
        }
    }

    vkUpdateDescriptorSets(m_device, 
            write_sets.size(), 
            write_sets.data(),
            0, nullptr);

	vkCmdBindDescriptorSets(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, 
            pipeline->get_pipeline_layout(), 0, desc_sets.size(), 
            desc_sets.data(), 0, nullptr);
    
    VkDevice device = m_device;
    m_fctx->get_deletion_queue().push_function([desc_allocator, device](){ 
        desc_allocator->clear_descriptors(device);
    });
}

void PassContext::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
    vkCmdDispatch(m_cmd, x, y, z);
}

VkClearColorValue check_clear_color(std::optional<Color> color) {
    VkClearColorValue clear;
    if(color.has_value()) {
        const Color& c = color.value();
        clear = { c.r, c.g, c.b, c.a };
    }
    return clear;
}

void PassContext::fill_attachments(VkRenderingInfo& info, GraphicsPass* pass) {
    std::vector<VkRenderingAttachmentInfo> color_attachment_info;
    VkRenderingAttachmentInfo depth_attachment_info;
    VkRenderingAttachmentInfo stencil_attachment_info;
    
    const GraphicsAttachments& attachments = pass->get_attachments();
    for(const RenderAttachment& ca : attachments.color_attachments) {
        FGTexture* fg_texture = m_textures.get(ca.handle);
        TextureResource* texture = fg_texture->get_resource();
        
        VkClearColorValue clear = check_clear_color(ca.clear_color);

        color_attachment_info.push_back(VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = texture->view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = util::to_vulkan(ca.load_op),
            .storeOp = util::to_vulkan(ca.store_op),
            .clearValue = { .color = { clear }} 
        });
    }
    
    const RenderAttachment& da = attachments.depth_attachment;
    const RenderAttachment& sa = attachments.stencil_attachment;
    
    
    TextureResource* depth_texture = m_textures.get(da.handle)->get_resource(); 
    TextureResource* stencil_texture;

    VkImageLayout depth_layout;
    VkImageLayout stencil_layout;
    if(attachments.is_combined()) { 
        depth_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencil_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        stencil_texture = depth_texture;
    } else {
        depth_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL; 
        stencil_layout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL; 

        stencil_texture = m_textures.get(sa.handle)->get_resource();
    }

    VkClearColorValue da_clear = check_clear_color(da.clear_color);
    VkClearColorValue sa_clear = check_clear_color(sa.clear_color);

    depth_attachment_info = (VkRenderingAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = depth_texture->view,
        .imageLayout = depth_layout,
        .loadOp = util::to_vulkan(da.load_op),
        .storeOp = util::to_vulkan(da.store_op),
        .clearValue = { .color = { da_clear }} 
    });

    stencil_attachment_info = (VkRenderingAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = stencil_texture->view,
        .imageLayout = stencil_layout,
        .loadOp = util::to_vulkan(sa.load_op),
        .storeOp = util::to_vulkan(sa.store_op),
        .clearValue = { .color = { sa_clear }} 
    });

    info.colorAttachmentCount = color_attachment_info.size();
	info.pColorAttachments = color_attachment_info.data();
	info.pDepthAttachment = &depth_attachment_info;
	info.pStencilAttachment = &stencil_attachment_info;
}


void PassContext::bind_graphics_pipeline(GraphicsPipeline* pipeline,
        DrawConfig config) {
    assert(m_pass->get_type() == PassType::Graphics);

    GraphicsPass* g_pass = static_cast<GraphicsPass*>(m_pass);
    const GraphicsPipelineDesc& desc = pipeline->get_desc();

	VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.pNext = nullptr,
		.renderArea = config.render_area,
		.layerCount = 1
	};
    fill_attachments(render_info, g_pass);

	vkCmdBeginRendering(m_cmd, &render_info);
	
	//set dynamic viewport and scissor
    vkCmdSetViewport(m_cmd, 0, 1, &config.viewport);
    vkCmdSetScissor(m_cmd, 0, 1, &config.scissor);

	vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            pipeline->get_pipeline());

    // TODO: BIND INDEX (and vertex???) BUFFER
    // ADD PUSH CONSTANTS

	vkCmdEndRendering(m_cmd);
}
