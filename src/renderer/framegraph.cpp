#include "foxglove/renderer/framegraph.h"

#include <iostream>

//
// FrameGraph
//

void FrameGraph::init(VulkanContext* ctx, Swapchain* swapchain) {
    m_ctx = ctx;
    m_swapchain = swapchain;
	
    /*
    //create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	};
	
    m_desc_allocator.init_pool(m_ctx->get_device(), 10, sizes);
    */
}

FGBufferHandle FrameGraph::create_buffer(const std::string& name,
        BufferDesc desc) {
    return m_buffers.create(name, desc);
}

FGTextureHandle FrameGraph::create_texture(const std::string& name,
        TextureDesc desc) {
    return m_textures.create(name, desc);
}

FGTextureHandle FrameGraph::register_external_texture(const std::string& name,
        TextureDesc desc, TextureResource* resource) {
    FGTextureHandle h = m_textures.create(name, desc, resource);
    return h;
}

PassBuilder FrameGraph::create_pass(const std::string& name, PassType type) {
    return PassBuilder(this, name, type);
}

void FrameGraph::add_pass(std::unique_ptr<Pass> pass) {
    m_passes.push_back(std::move(pass));
    m_dirty = true;
}
// TODO: add last_pass tracking when compiling


void FrameGraph::compile() {
    // validate each pass
    // build exec order
   
    build_dependencies();

    //collect_descriptors();
    
    // what else

    m_dirty = false;
}


void add_binding(int pass_num,
        FGResource& resource,
        ResourceAccess access,
        std::vector<std::vector<size_t>>& adj_list,
        std::unordered_map<FGResource*, size_t>& last_writer,
        std::vector<int> in_degree) {
    // read
    if(access & ResourceAccess::Read) {
        auto it = last_writer.find(&resource);
        if(it != last_writer.end()) {
            adj_list[it->second].push_back(pass_num);
            in_degree[pass_num]++;
        }
    }
        
    // write
    if(access & ResourceAccess::Write) {
        last_writer[&resource] = pass_num;
    }
}

void add_pass_dependency(Pass* producer, Pass* consumer) {
    std::vector<Pass*>& producers = consumer->get_producers();
    
    auto itr = std::find(producers.begin(), producers.end(), producer); 
    // if not found, add it
    if(itr == producers.end()) {
        producers.push_back(producer);
    }
}

// TODO: skip uav barrier?

void FrameGraph::build_dependencies() {
    // build an adjacency list graph
    std::vector<std::vector<size_t>> adj_list(m_passes.size());
    std::unordered_map<FGResource*, size_t> last_writer;
    std::vector<int> in_degree(m_passes.size(), 0);

    for(size_t i = 0; i < m_passes.size(); i++) {
        Pass* pass = m_passes[i].get();

        // add an edge if a read depends on a last write
        // TODO: THIS ASSUMES PASSES COME IN ORDER
        // DONT, CONSIDER OTHER ISSUES
        
        pass->enumerate_buffers([this, pass](const BufferBinding& b) {
            FGBuffer* buffer = m_buffers.get(b.handle);
            Pass* last = buffer->get_last_writer();

            if(last != nullptr) {
                add_pass_dependency(last, pass);
            }
        });
        
        // in the future this will have to be done per subresource
        pass->enumerate_textures([this, pass](const TextureBinding& t) {
            FGTexture* texture = m_textures.get(t.handle);
            Pass* last = texture->get_last_writer();

            if(last != nullptr) {
                add_pass_dependency(last, pass);
            }
        });
    }

    // TODO: CULL
}

/*
void FrameGraph::collect_pass_bindings(DescriptorLayoutBuilder& builder,
        Pass* pass) {
    int binding_num = 0;
    pass->enumerate_buffers([&builder, &binding_num](const BufferBinding& b) {
        // TODO: edit to add deduction
        builder.add_binding(binding_num++, 
                util::deduce_descriptor_type(b.usage));
    });

    pass->enumerate_textures([&builder, &binding_num](const TextureBinding& t) {
        // TODO: edit to add deduction
        builder.add_binding(binding_num++,
                util::deduce_descriptor_type(t.usage));
    });
}


void FrameGraph::collect_descriptors() {
    std::vector<uint32_t> pass_ids;
    std::vector<VkDescriptorSetLayout> layouts;
    for(size_t i = 0; i < m_passes.size(); i++) {
        Pass* pass = m_passes[i].get();
        
        DescriptorLayoutBuilder builder;
        if(pass->get_type() == PassType::Compute) {
            pass_ids.push_back(i);
            collect_pass_bindings(builder, pass);
            layouts.push_back(builder.build(m_ctx->get_device(), 
                        VK_SHADER_STAGE_COMPUTE_BIT));
        } else if(pass->get_type() == PassType::Graphics) {
            pass_ids.push_back(i);
            collect_pass_bindings(builder, pass);
            // TODO: NARROW THIS MAYBE
            layouts.push_back(builder.build(m_ctx->get_device(), 
                        VK_SHADER_STAGE_ALL_GRAPHICS));
        }
    }
    
    // change from pass_ids to batch struct of vectors?
    if(!layouts.empty()) {
        std::vector<VkDescriptorSet> sets = m_desc_allocator.allocate(
                m_ctx->get_device(), layouts);
        for(size_t i = 0; i < sets.size(); i++) {
            Pass* pass = m_passes[pass_ids[i]].get();
            pass->set_descriptor_info(layouts[i], sets[i]);
        }
    }
}
*/

void FrameGraph::allocate_resources(FrameContext& fctx) {
    // first collect
    std::vector<FGBuffer*> transient_buffers;
    std::vector<FGTexture*> transient_textures;
    for(size_t i = 0; i < m_passes.size(); i++) {
        Pass* pass = m_passes[i].get();
        
        // for(const BufferBinding& b : pass->get_buffers()) {
        pass->enumerate_buffers([this, &transient_buffers]
        (const BufferBinding& b) {
            FGBuffer* buffer = m_buffers.get(b.handle);
            if(buffer->is_transient() && !buffer->collected()) {
                transient_buffers.push_back(buffer);
                buffer->collect();
            }
        });

        //for(const TextureBinding& t: pass->get_textures()) {
        pass->enumerate_textures([this, &transient_textures]
        (const TextureBinding& t) {
            FGTexture* texture = m_textures.get(t.handle); 
            if(texture->is_transient() && !texture->collected()) {
                transient_textures.push_back(texture);
                texture->collect();
            }
        });
    }

    // then allocate
    for(FGBuffer* buffer : transient_buffers) {
        // consider pooling + caching
        BufferResource* resource = new BufferResource;
        resource->create(m_ctx->get_device(),
                m_ctx->get_allocator(),
                buffer->get_desc());
        buffer->set_resource(resource);

        fctx.get_deletion_queue().push_function([&, resource]() {
            resource->destroy({
                m_ctx->get_device(), 
                m_ctx->get_allocator()
            });
        });
    }

    for(FGTexture* texture : transient_textures) {
        TextureResource* resource = new TextureResource;
        resource->create(m_ctx->get_device(),
                m_ctx->get_allocator(),
                texture->get_desc());
        texture->set_resource(resource);

        fctx.get_deletion_queue().push_function([&, resource]() {
            resource->destroy({
                m_ctx->get_device(), 
                m_ctx->get_allocator()
            });
        });
    }
}

void add_buffer_barrier(const BufferBinding& bb, Pass* pass) {

}

void add_texture_barrier(const TextureBinding& tb, Pass* pass) {
}

void FrameGraph::collect_pass_barriers() {
    for(size_t i = 0; i < m_passes.size(); i++) {
        Pass* pass = m_passes[i].get();

        if(pass->is_culled()) {
            continue;
        }
        
        pass->enumerate_buffers([this, pass](const BufferBinding& b) {
            FGBuffer* buffer = m_buffers.get(b.handle);

            BufferTransitionInfo bti = {
                buffer->get_handle(),
                buffer->get_usage(),         // src
                buffer->get_access(),        // src
                b.usage,                   // dst
                b.access                   // dst
            };

            pass->add_buffer_transition(bti);

            // don't forget to change state
            buffer->set_usage(b.usage);
            buffer->set_access(b.access);
        });

        pass->enumerate_textures([this, pass](const TextureBinding& t) {
            FGTexture* texture = m_textures.get(t.handle); 
            TextureTransitionInfo tti = {
                texture->get_handle(),
                texture->get_usage(),        // src
                texture->get_access(),       // src
                t.usage,                   // dst
                t.access                   // dst
            };

            pass->add_texture_transition(tti);

            // don't forget to change state
            texture->set_usage(t.usage);
            texture->set_access(t.access);
        });
    }
}


// Unreal uses a transition_create_queue, added to by add_x_barrier
void FrameGraph::compile_pass_barriers(FrameContext& fctx) {
    for(size_t i = 0; i < m_passes.size(); i++) {
        Pass* pass = m_passes[i].get();

        for(BufferTransitionInfo bti : pass->get_buffer_transitions()) {
            FGBuffer* buffer = m_buffers.get(bti.handle);
            BufferResource* br = buffer->get_resource();
  
            VkPipelineStageFlags2 src_stage = util::deduce_pipeline_flags(
                    bti.src_usage);
            VkAccessFlags2 src_access = util::deduce_access_flags(
                    bti.src_usage);

            VkPipelineStageFlags2 dst_stage = util::deduce_pipeline_flags(
                    bti.dst_usage);
            VkAccessFlags2 dst_access = util::deduce_access_flags(
                    bti.dst_usage);

            VkBufferMemoryBarrier2 buf_b = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = src_stage,
                .srcAccessMask = src_access,
                .dstStageMask = dst_stage,
                .dstAccessMask = dst_access,
                .buffer = br->buffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            };

            pass->add_vk_buffer_barrier(buf_b);

        }
        
        for(TextureTransitionInfo tti : pass->get_texture_transitions()) {
            FGTexture* texture = m_textures.get(tti.handle);
            TextureResource* tr = texture->get_resource();


            VkPipelineStageFlags2 src_stage = util::deduce_pipeline_flags(
                    tti.src_usage);
            VkAccessFlags2 src_access = util::deduce_access_flags(
                    tti.src_usage);
            VkImageLayout src_layout = util::deduce_layout(tti.src_usage);

            VkPipelineStageFlags2 dst_stage = util::deduce_pipeline_flags(
                    tti.dst_usage);
            VkAccessFlags2 dst_access = util::deduce_access_flags(
                    tti.dst_usage);
            VkImageLayout dst_layout = util::deduce_layout(tti.dst_usage);

            VkImageAspectFlags aspect_mask;
            if (dst_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
                aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
            } else {
                aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            }
            //std::cout << texture->get_name() << ": " << src_layout << " to " << dst_layout << std::endl;

            VkImageMemoryBarrier2 img_b = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = src_stage, 
                .srcAccessMask = src_access,
                .dstStageMask = dst_stage,
                .dstAccessMask = dst_access,
                .oldLayout = src_layout,
                .newLayout = dst_layout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = tr->image,
                .subresourceRange = {
                    .aspectMask = aspect_mask,
                    .baseMipLevel = 0,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS
                }
            };


            pass->add_vk_image_barrier(img_b);
        }
        
        // convert swapchain image to TransferDst
        if(pass->get_type() == PassType::Present) {
            FGTextureHandle handle = fctx.get_swapchain_handle();
            FGTexture* texture = m_textures.get(handle);
            TextureResource* tr = texture->get_resource();

            VkImageMemoryBarrier2 img_b = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE, 
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                //.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = tr->image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS
                    }
    };

            pass->add_vk_image_barrier(img_b);
        }
    }
}


VkRenderingInfo FrameGraph::get_rendering_info(GraphicsPass* pass) {
    if(pass->b_attachments) {
        return pass->m_rendering_info;
    }

    VkRenderingAttachmentInfo* depth_ptr = nullptr;
    VkRenderingAttachmentInfo* stencil_ptr = nullptr;
    
    std::vector<VkRenderingAttachmentInfo>& color_attachment_info = pass
        ->m_color_attachment_info;
    VkRenderingAttachmentInfo& depth_attachment_info = pass
        ->m_depth_attachment_info;
    VkRenderingAttachmentInfo& stencil_attachment_info = pass
        ->m_stencil_attachment_info;
    
    const GraphicsPassInfo& info = pass->get_info();
    for(const ColorAttachment& ca : info.color_attachments) {
        FGTexture* fg_texture = m_textures.get(ca.handle);
        TextureResource* texture = fg_texture->get_resource();
        
        VkClearColorValue clear;
        if(ca.clear_value.has_value()) {
            const Color& c = ca.clear_value.value();
            clear = { c.r, c.g, c.b, c.a };
        }
        
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
    
    if (info.depth_attachment.has_value()) {
        const DepthAttachment& da = info.depth_attachment.value();
        TextureResource* depth_texture = m_textures.get(da.handle)
            ->get_resource();
        
        VkImageLayout layout = info.is_combined() 
            ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
        depth_attachment_info = VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = depth_texture->view,
            .imageLayout = layout,
            .loadOp = util::to_vulkan(da.load_op),
            .storeOp = util::to_vulkan(da.store_op),
            .clearValue = { .depthStencil = {
                .depth = da.clear_value.has_value() 
                    ? da.clear_value.value() 
                    : 0.0f,
                .stencil = 0
            }}
        };

        depth_ptr = &depth_attachment_info;
    }
    
    if (info.stencil_attachment.has_value()) {
        const StencilAttachment& sa = info.stencil_attachment.value();
        TextureResource* stencil_texture = m_textures.get(sa.handle)
            ->get_resource();
        
        VkImageLayout layout = info.is_combined() 
            ? VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
        stencil_attachment_info = VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = stencil_texture->view,
            .imageLayout = layout,
            .loadOp = util::to_vulkan(sa.load_op),
            .storeOp = util::to_vulkan(sa.store_op),
            .clearValue = { .depthStencil = {
                .depth = 0,
                .stencil = sa.clear_value.has_value() 
                    ? sa.clear_value.value()
                    : 0u
            }}
        };

        stencil_ptr = &stencil_attachment_info;
    }
    
    pass->b_attachments = true;
    pass->m_rendering_info = VkRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .renderArea = info.render_area,
        .layerCount = info.layer_count,
        .colorAttachmentCount = static_cast<uint32_t>(
                color_attachment_info.size()),
        .pColorAttachments = color_attachment_info.data(),
        .pDepthAttachment = depth_ptr,
        .pStencilAttachment = stencil_ptr
    };

    return pass->m_rendering_info;
}

void FrameGraph::execute(FrameContext& fctx) {
    //if(m_dirty) {
    compile();

    // steps that use the underlying resources
    allocate_resources(fctx);
    
    collect_pass_barriers();
    compile_pass_barriers(fctx);

    // execute passes in order
    for(std::unique_ptr<Pass>& pass : m_passes) {
        std::vector<VkBufferMemoryBarrier2>& buffer_barriers = 
            pass->get_vk_buffer_barriers();
        std::vector<VkImageMemoryBarrier2>& image_barriers = 
            pass->get_vk_image_barriers();

        if(!buffer_barriers.empty() || !image_barriers.empty()) {
            VkDependencyInfo dep_info = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size()),
                .pBufferMemoryBarriers = buffer_barriers.data(),
                .imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
                .pImageMemoryBarriers = image_barriers.data()
            };

            vkCmdPipelineBarrier2(fctx.get_cmd_buffer(), &dep_info);
        }

        
        PassContext ctx(m_ctx->get_device(), 
                m_buffers, m_textures, &fctx, 
                pass.get());

        if(pass->get_type() == PassType::Graphics) {
            GraphicsPass* g_pass = static_cast<GraphicsPass*>(pass.get());
            VkRenderingInfo render_info = get_rendering_info(g_pass);
            VkCommandBuffer cmd = fctx.get_cmd_buffer();
            
            vkCmdBeginRendering(cmd, &render_info);
            pass->execute(ctx);
            vkCmdEndRendering(cmd);
        } else {
            pass->execute(ctx);
        }
    }
    reset();
}


// dont delete right away, send to deletion queue
void FrameGraph::reset() {
    m_buffers.reset();
    m_textures.reset();
    m_passes.clear();
}

