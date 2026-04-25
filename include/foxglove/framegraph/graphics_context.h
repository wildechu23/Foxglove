#pragma once

#include "foxglove/resources/pipeline/pipeline.h"
#include "foxglove/framegraph/fg_resource.h"

class FrameGraph;

class GraphicsContext {
public:
    GraphicsContext(VkCommandBuffer cmd, GraphicsPipeline* pipeline,
            FrameGraph* fg, VkRect2D default_rect) : 
        m_cmd(cmd), m_pipeline(pipeline), m_fg(fg),
        m_default_rect(default_rect) {}

    GraphicsContext& bind_viewport(VkViewport viewport);
    GraphicsContext& bind_scissor(VkRect2D scissor);
    
    /*
    GraphicsContext& bind_vertex_buffer(FGBufferHandle handle, 
            uint32_t binding = 0, VkDeviceSize offset = 0);
            */

    GraphicsContext& bind_index_buffer(FGBufferHandle handle, 
            VkDeviceSize offset = 0);
    GraphicsContext& push_constants(const void* data, size_t size, 
            uint32_t offset = 0);
    
    template<typename T>
    GraphicsContext& push_constant(const T& data, uint32_t offset = 0) {
        return push_constants(&data, sizeof(T), offset);
    }
    
    // Draw commands
    void draw(uint32_t vertex_count, uint32_t instance_count = 1,
              uint32_t fvertex = 0, uint32_t finstance = 0);
    
    void draw_indexed(uint32_t index_count, uint32_t instance_count = 1,
                      uint32_t findex = 0, int32_t vertex_offset = 0,
                      uint32_t finstance = 0);
private:
    void check_defaults();

    VkCommandBuffer m_cmd;
    GraphicsPipeline* m_pipeline;

    FrameGraph* m_fg;

    bool b_viewport = false;
    bool b_scissor = false;
    
    // default rectangle for viewport and scissor: render area
    VkRect2D m_default_rect;
};
