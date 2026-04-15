#pragma once

#include "foxglove/resources/pipeline.h"
#include "foxglove/resources/fg_resource_types.h"



class GraphicsContext {
public:
    GraphicsContext(VkCommandBuffer cmd, GraphicsPipeline* pipeline,
            FGBufferRegistry& buffers, FGTextureRegistry& textures) : 
        m_cmd(cmd), m_pipeline(pipeline), 
        m_buffers(buffers), m_textures(textures) {}

    GraphicsContext& bind_viewport(VkViewport viewport);
    GraphicsContext& bind_scissor(VkRect2D scissor);
    
    GraphicsContext& bind_vertex_buffer(FGBufferHandle handle, 
            uint32_t binding = 0, VkDeviceSize offset = 0);
    
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
    VkCommandBuffer m_cmd;
    GraphicsPipeline* m_pipeline;
    FGBufferRegistry& m_buffers;
    FGTextureRegistry& m_textures;
};
