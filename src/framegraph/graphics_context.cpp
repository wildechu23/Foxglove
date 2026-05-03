#include "foxglove/framegraph/graphics_context.h"
#include "foxglove/framegraph/framegraph.h"

GraphicsContext& GraphicsContext::bind_viewport(VkViewport viewport) {
    vkCmdSetViewport(m_cmd, 0, 1, &viewport);
    b_viewport = true;
    return *this;
}

GraphicsContext& GraphicsContext::bind_scissor(VkRect2D scissor) {
    vkCmdSetScissor(m_cmd, 0, 1, &scissor);
    b_scissor = true;
    return *this;
}

/*
GraphicsContext& GraphicsContext::bind_vertex_buffer(FGBufferHandle handle,
        uint32_t binding, VkDeviceSize offset) {
    BufferResource* b = m_fg->get_buffer(handle)->get_resource();
    vkCmdBindVertexBuffers(m_cmd, binding, 1, &b->buffer, &offset);
    return *this;
}*/

GraphicsContext& GraphicsContext::bind_index_buffer(FGBufferHandle handle,
        VkDeviceSize offset) {
    BufferResource* b = m_fg->get_buffer(handle)->get_resource_ptr();
    vkCmdBindIndexBuffer(m_cmd, b->buffer, offset, VK_INDEX_TYPE_UINT32);
    return *this;
}

GraphicsContext& GraphicsContext::push_constants(const void* data, 
        size_t size, uint32_t offset) {
    vkCmdPushConstants(m_cmd, 
            m_pipeline->get_pipeline_layout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            offset, size, data);
    return *this;
}

void GraphicsContext::check_defaults() {
    // check viewport
    if(!b_viewport) {
        VkViewport viewport = {
            static_cast<float>(m_default_rect.offset.x),
            static_cast<float>(m_default_rect.offset.y),
            static_cast<float>(m_default_rect.extent.width),
            static_cast<float>(m_default_rect.extent.height),
            0.f,
            1.f
        };
        vkCmdSetViewport(m_cmd, 0, 1, &viewport);
    }
    // check scissor
    if(!b_scissor) {
        vkCmdSetScissor(m_cmd, 0, 1, &m_default_rect);
    }
}

void GraphicsContext::draw(uint32_t vertex_count, uint32_t instance_count,
                       uint32_t fvertex, uint32_t finstance) {
    check_defaults();
    vkCmdDraw(m_cmd, vertex_count, instance_count, fvertex, finstance);
}

void GraphicsContext::draw_indexed(
        uint32_t index_count, uint32_t instance_count,
        uint32_t findex, int32_t vertex_offset, uint32_t finstance) {
    check_defaults();
    vkCmdDrawIndexed(m_cmd, index_count, instance_count, findex, 
                     vertex_offset, finstance);
}
