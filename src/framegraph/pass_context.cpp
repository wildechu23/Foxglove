#include "foxglove/framegraph/pass_context.h"

#include "foxglove/framegraph/frame_context.h"
#include "foxglove/framegraph/framegraph.h"
#include "foxglove/core/math.h"

PassContext::PassContext(FrameGraph* fg, FrameContext* fctx,
            VulkanContext* ctx, Pass* pass)
    : m_fg(fg), m_fctx(fctx), m_ctx(ctx), m_pass(pass) {
    m_device = m_ctx->get_device(); 
    m_cmd = fctx->get_cmd_buffer();
}

void PassContext::bind_compute_pipeline(ComputePipeline* pipeline) {
    assert(m_pass->get_type() == PassType::Compute);
    // bind pipeline
	vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, 
            pipeline->get_pipeline());
    //update_descriptor_sets(pipeline, VK_PIPELINE_BIND_POINT_COMPUTE);
    update_descriptor_heap(); 
}

void PassContext::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
    vkCmdDispatch(m_cmd, x, y, z);
}

GraphicsContext PassContext::bind_graphics_pipeline(GraphicsPipeline* pipeline) {
    assert(m_pass->get_type() == PassType::Graphics);

	vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            pipeline->get_pipeline());
    
    //update_descriptor_sets(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);
    update_descriptor_heap(); 
    
    GraphicsPass* g_pass = static_cast<GraphicsPass*>(m_pass);
    VkRect2D render_area = g_pass->get_info().render_area;
    
    return GraphicsContext(m_ctx, m_cmd, pipeline, m_fg, render_area);
}
/*
void PassContext::update_descriptor_sets(Pipeline* pipeline, 
        VkPipelineBindPoint bind_point) {
    const std::vector<BindingGroup>& bind_groups = m_pass->get_bind_groups();

    if(bind_groups.empty()) return;
    
    // build writes
    DescriptorAllocator* desc_allocator = &m_fctx->get_descriptor_allocator();
    std::vector<VkDescriptorSet> desc_sets = desc_allocator->allocate(
            m_device, pipeline->m_descriptor_layouts);

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
            FGBuffer* fg_buffer = m_fg->get_buffer(bb.handle);
            BufferResource* buffer = fg_buffer->get_resource_ptr();

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
            FGTexture* fg_texture = m_fg->get_texture(tb.handle);
            TextureResource* texture = fg_texture->get_resource_ptr();

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

	vkCmdBindDescriptorSets(m_cmd, bind_point, 
            pipeline->get_pipeline_layout(), 0, desc_sets.size(), 
            desc_sets.data(), 0, nullptr);
    
    VkDevice device = m_device;
    m_fctx->get_deletion_queue().push_function([desc_allocator, device](){ 
        desc_allocator->clear_descriptors(device);
    });
}
*/

void PassContext::update_descriptor_heap() {
    // TODO: THIS SHOULD GO EARLIER
    const BindingGroup& bindings = m_pass->get_bindings();

    if(bindings.buffers.empty() && bindings.textures.empty()) return;
    
    DescriptorHeapAllocator& heap = *m_fctx->get_descriptor_heap();
    
    std::vector<BufferKey> buffers;
    std::vector<TextureKey> textures;
    std::vector<SamplerKey> samplers;

    std::vector<BufferKey> transient_buffers;
    std::vector<TextureKey> transient_textures;
    
    for(const BufferBinding& bb : bindings.buffers) {
        FGBuffer* fg_buffer = m_fg->get_buffer(bb.handle);
        BufferKey key = {
            fg_buffer->get_resource(),
            bb.usage,
        };
        if(!heap.has_resource(key)) {
            if(fg_buffer->is_transient()) {
                transient_buffers.push_back(key);
            } else buffers.push_back(key);
        }
    }

    for(const TextureBinding& tb : bindings.textures) {
        FGTexture* fg_texture = m_fg->get_texture(tb.handle);
        TextureKey key = {
            fg_texture->get_resource(),
            tb.usage,
            tb.access,
        };
        if(!heap.has_resource(key)) {
            if(fg_texture->is_transient()) {
                transient_textures.push_back(key);
            } else textures.push_back(key);
        }
    }

    for(const SamplerBinding& sb : bindings.samplers) {
        FGSampler* fg_sampler = m_fg->get_sampler(sb.handle);
        SamplerKey key = fg_sampler->get_resource(); 
        if(!heap.has_resource(key)) {
            samplers.push_back(key);
        }
    }

    heap.add_descriptors(buffers, textures, samplers,
            transient_buffers, transient_textures);

    heap.write_pending();

    // now write into data the size of total_bindings * uint32_t
    std::vector<uint32_t> push_data(bindings.size());
    
    for(const BufferBinding& bb : bindings.buffers) {
        BufferHandle handle = m_fg->get_buffer(bb.handle)->get_resource();
        push_data[bb.binding] = heap.get_index({handle, bb.usage});
    }

    for(const TextureBinding& tb : bindings.textures) {
        TextureHandle handle = m_fg->get_texture(tb.handle)->get_resource();
        push_data[tb.binding] = heap.get_index({handle, 
                tb.usage, tb.access});
    }

    for(const SamplerBinding& sb : bindings.samplers) {
        SamplerHandle handle = m_fg->get_sampler(sb.handle)->get_resource();
        push_data[sb.binding] = heap.get_index(handle);
    }

    VkPushDataInfoEXT push_data_info = {
        .sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
        .pNext = NULL,
        .offset = 0,
        .data = {
            .address = push_data.data(),
            .size = align_up(push_data.size() * sizeof(uint32_t), 4)
        }
    };

    m_ctx->vkCmdPushDataEXT(m_cmd, &push_data_info);
    
    if(bindings.has_resource()) heap.bind_resource_heap(m_cmd);
    if(bindings.has_sampler()) heap.bind_sampler_heap(m_cmd);

    m_fctx->get_deletion_queue().push_function([&heap, transient_buffers]() {
        for(BufferKey k : transient_buffers) {
            heap.free_resource(k);
        }
    });

    m_fctx->get_deletion_queue().push_function([&heap, transient_textures]() {
        for(TextureKey k : transient_textures) {
            heap.free_resource(k);
        }
    });
}

// TODO: for compute pass, maybe move to compute builder
void PassContext::push_constant(const void* data,
        size_t size, uint32_t offset) {
    VkPushDataInfoEXT push_data_info = {
        .sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
        .pNext = NULL,
        .offset = offset,
        .data = {
            .address = data,
            .size = static_cast<uint32_t>(align_up(size, 4))
        }
    };

    m_ctx->vkCmdPushDataEXT(m_cmd, &push_data_info);
}

