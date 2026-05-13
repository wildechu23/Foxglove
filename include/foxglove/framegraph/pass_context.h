#pragma once

#include "foxglove/framegraph/attachments.h"
#include "foxglove/framegraph/graphics_context.h"
#include "foxglove/framegraph/pass.h"
#include "foxglove/framegraph/util.h"

class FrameContext;
class FrameGraph;

class PassContext {
public:
    PassContext(FrameGraph* fg, FrameContext* fctx, VulkanContext* ctx, Pass* pass);

    VkCommandBuffer get_cmd() { return m_cmd; }
    TextureResource* get_swapchain() const;
    
    void bind_compute_pipeline(ComputePipeline* pipeline);
    void dispatch_compute(uint32_t x, uint32_t y, uint32_t z);

    GraphicsContext bind_graphics_pipeline(GraphicsPipeline* pipeline);

    void push_constant(const void* data, size_t size, 
            uint32_t offset = 0);
    
    template<typename T>
    void push_constant(const T& data, uint32_t offset = 0) {
        return push_constant(&data, sizeof(T), offset);
    }
private:
    //void update_descriptor_sets(Pipeline* pipeline, 
    //        VkPipelineBindPoint bind_point);

    void update_descriptor_heap();

    VkRenderingInfo build_rendering_info(GraphicsPass* pass);

    FrameGraph* m_fg;
    VkCommandBuffer m_cmd;
    FrameContext* m_fctx;
    
    VulkanContext* m_ctx;
    VkDevice m_device;

    Pass* m_pass;

    friend class PassBuilder;
};

