#pragma once

#include "foxglove/framegraph/attachments.h"
#include "foxglove/framegraph/graphics_context.h"
#include "foxglove/framegraph/pass.h"
#include "foxglove/framegraph/util.h"

class FrameContext;
class FrameGraph;

class PassContext {
public:
    PassContext(FrameGraph* fg, FrameContext* fctx, Pass* pass);

    VkCommandBuffer get_cmd() { return m_cmd; }
    FGTextureHandle get_swapchain_handle() const;
    
    void bind_compute_pipeline(ComputePipeline* pipeline);
    void dispatch_compute(uint32_t x, uint32_t y, uint32_t z);

    GraphicsContext bind_graphics_pipeline(GraphicsPipeline* pipeline);
private:
    void update_descriptor_sets(Pipeline* pipeline, 
            VkPipelineBindPoint bind_point);

    VkRenderingInfo build_rendering_info(GraphicsPass* pass);

    VkDevice m_device;
    FrameGraph* m_fg;
    VkCommandBuffer m_cmd;

    FrameContext* m_fctx;

    Pass* m_pass;
};

