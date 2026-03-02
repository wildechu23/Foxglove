#include "foxglove/renderer/framegraph.h"

//
// FrameGraph
//

void FrameGraph::init(VulkanContext* ctx, Swapchain* swapchain) {
    m_ctx = ctx;
    m_swapchain = swapchain;
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
    // transient resources?
    // build exec order
   
    build_dependencies();

    allocate_resources();

    collect_pass_barriers();
    compile_pass_barriers();
    
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
        for(const BufferBinding& b : pass->get_buffers()) {
            Pass* last = b.buffer.get_last_writer();

            if(last != nullptr) {
                add_pass_dependency(last, pass);
            }
        }
        
        // in the future this will have to be done per subresource
        for(const TextureBinding& t : pass->get_textures()) {
            Pass* last = t.texture.get_last_writer();

            if(last != nullptr) {
                add_pass_dependency(last, pass);
            }
        }
    }

    // TODO: CULL
}

void collect_buffer(std::vector<FGBuffer*> ctx, Pass* pass, 
        FGBuffer& buffer) {
    // check if first pass?
    
    // collect allocations
    if(buffer.is_transient()) {
        ctx.push_back(&buffer);
    }
}

void collect_texture(std::vector<FGTexture*> ctx, Pass* pass,
        FGTexture& texture) {
    // check if first pass?
    
    // collect allocations
    if(texture.is_transient()) {
        ctx.push_back(&texture);
    }
}

void FrameGraph::allocate_resources() {
    // first collect
    std::vector<FGBuffer*> transient_buffers;
    std::vector<FGTexture*> transient_textures;
    for(size_t i = 0; i < m_passes.size(); i++) {
        Pass* pass = m_passes[i].get();
        
        for(const BufferBinding& b: pass->get_buffers()) {
            collect_buffer(transient_buffers, pass, b.buffer);
        }

        for(const TextureBinding& t: pass->get_textures()) {
            collect_texture(transient_textures, pass, t.texture);
        }
    }

    // then allocate
    for(FGBuffer* buffer : transient_buffers) {
        // consider pooling + caching
        BufferResource* resource = new BufferResource;
        resource->create(m_ctx->get_device(),
                m_ctx->get_allocator(),
                buffer->get_desc());
        buffer->set_resource(resource);
    }

    for(FGTexture* texture : transient_textures) {
        TextureResource* resource = new TextureResource;
        resource->create(m_ctx->get_device(),
                m_ctx->get_allocator(),
                texture->get_desc());
        texture->set_resource(resource);
    }
}

void add_buffer_barrier(const BufferBinding& bb, Pass* pass) {
    FGBuffer& buffer = bb.buffer;

    BufferTransitionInfo bti = {
        buffer.get_handle(),
        buffer.get_usage(),         // src
        buffer.get_access(),        // src
        bb.usage,                   // dst
        bb.access                   // dst
    };

    pass->add_buffer_transition(bti);
    
    // don't forget to change state
    buffer.set_usage(bb.usage);
    buffer.set_access(bb.access);
}

void add_texture_barrier(const TextureBinding& tb, Pass* pass) {
    FGTexture& texture = tb.texture;

    TextureTransitionInfo tti = {
        texture.get_handle(),
        texture.get_usage(),        // src
        texture.get_access(),       // src
        tb.usage,                   // dst
        tb.access                   // dst
    };

    pass->add_texture_transition(tti);
    
    // don't forget to change state
    texture.set_usage(tb.usage);
    texture.set_access(tb.access);
}

void FrameGraph::collect_pass_barriers() {
    for(size_t i = 0; i < m_passes.size(); i++) {
        Pass* pass = m_passes[i].get();

        if(pass->is_culled()) {
            continue;
        }
        
        for(const BufferBinding& b : pass->get_buffers()) {
            add_buffer_barrier(b, pass); 
        }

        for(const TextureBinding& t : pass->get_textures()) {
            add_texture_barrier(t, pass);
        }
    }
}

// Unreal uses a transition_create_queue, added to by add_x_barrier
void FrameGraph::compile_pass_barriers() {
    for(size_t i = 0; i < m_passes.size(); i++) {
        Pass* pass = m_passes[i].get();

        for(BufferTransitionInfo bti : pass->get_buffer_transitions()) {
            FGBuffer* buffer = m_buffers.get(bti.handle);
            BufferResource* br = buffer->get_resource();
  
            VkPipelineStageFlags2 src_stage = deduce_pipeline_flags(
                    bti.src_usage, pass->get_type());
            VkAccessFlags2 src_access = deduce_access_flags(
                    bti.src_usage);

            VkPipelineStageFlags2 dst_stage = deduce_pipeline_flags(
                    bti.dst_usage, pass->get_type());
            VkAccessFlags2 dst_access = deduce_access_flags(
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

            VkPipelineStageFlags2 src_stage = deduce_pipeline_flags(
                    tti.src_usage, pass->get_type());
            VkAccessFlags2 src_access = deduce_access_flags(
                    tti.src_usage);
            VkImageLayout src_layout = deduce_layout(tti.src_usage);

            VkPipelineStageFlags2 dst_stage = deduce_pipeline_flags(
                    tti.dst_usage, pass->get_type());
            VkAccessFlags2 dst_access = deduce_access_flags(
                    tti.dst_usage);
            VkImageLayout dst_layout = deduce_layout(tti.dst_usage);

            VkImageAspectFlags aspect_mask;
            if (dst_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
                aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
            } else {
                aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            VkImageMemoryBarrier2 img_b = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = src_stage, 
                .srcAccessMask = src_access,
                .dstStageMask = dst_stage,
                .dstAccessMask = dst_access,
                .oldLayout = src_layout,
                .newLayout = dst_layout,
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
    }
}

void FrameGraph::execute(FrameContext fctx) {
    if(m_dirty) {
        compile();
    }


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

        pass->execute(fctx.pass_view());
    }
}

// DEDUCE FLAGS
VkAccessFlags2 FrameGraph::deduce_access_flags(BufferUsage usage) {
    switch(usage) {
        // TODO: FIGURE OUT READ WRITE
        case BufferUsage::StorageBuffer:
            return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | 
                VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
        case BufferUsage::UniformBuffer:
            return VK_ACCESS_2_UNIFORM_READ_BIT;
        case BufferUsage::VertexBuffer:
            return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        case BufferUsage::IndexBuffer:
            return VK_ACCESS_2_INDEX_READ_BIT;
        default:
            return VK_ACCESS_2_NONE;
    }
}

VkAccessFlags2 FrameGraph::deduce_access_flags(TextureUsage usage) {
    switch(usage) {
        // FIGURE OUT READ WRITE
        case TextureUsage::ColorAttachment:
            return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
        case TextureUsage::DepthAttachment:
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case TextureUsage::InputAttachment:
            return VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;
        case TextureUsage::StorageImage:
            return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT |
                VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
        default:
            return VK_ACCESS_2_NONE;
    }
}

// TODO: FIGURE OUT HOW DEPTH/STENCIL ATTACHMENT WORKS
VkImageLayout FrameGraph::deduce_layout(TextureUsage usage) {
    switch(usage) {
        case TextureUsage::ColorAttachment:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 
        case TextureUsage::DepthAttachment:
            // TODO: CONSIDER OPTIONS FOR OTHER LAYOUTS
            // IE. DEPTH, STENCIL, + READ ONLY OPTIONS
            return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        case TextureUsage::InputAttachment:
            // TODO: CONSIDER OTHER READ ONLY LAYOUTS
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case TextureUsage::StorageImage:
            // TODO: IS THIS OPTIMIZABLE?
            return VK_IMAGE_LAYOUT_GENERAL;
        default:
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VkPipelineStageFlags2 FrameGraph::deduce_pipeline_flags(BufferUsage usage, PassType type) {
    switch(type) {
        case PassType::Graphics:
            switch(usage) {
                case BufferUsage::StorageBuffer:
                    return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                case BufferUsage::UniformBuffer:
                    return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                case BufferUsage::VertexBuffer:
                    return VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
                case BufferUsage::IndexBuffer:
                    return VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
            }
        case PassType::Compute:
            return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        case PassType::Transfer:
            // TODO: NARROW IF NEEDED
            return VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
        case PassType::Present:
            // BOTTOM OF PIPE OBSOLETE
            return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        default:
            return VK_PIPELINE_STAGE_2_NONE;
    }
}

VkPipelineStageFlags2 FrameGraph::deduce_pipeline_flags(TextureUsage usage, PassType type) {
    switch(type) {
        case PassType::Graphics:
            switch(usage) {
                case TextureUsage::ColorAttachment:
                    return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                case TextureUsage::DepthAttachment:
                    return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | 
                        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                case TextureUsage::InputAttachment:
                    return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                case TextureUsage::StorageImage:
                    return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
            }
        case PassType::Compute:
            return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        case PassType::Transfer:
            // TODO: NARROW IF NEEDED
            return VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
        case PassType::Present:
            // BOTTOM OF PIPE OBSOLETE
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        default:
            return VK_PIPELINE_STAGE_2_NONE;
    }
}


