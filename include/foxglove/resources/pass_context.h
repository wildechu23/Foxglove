#pragma once

#include "foxglove/resources/frame_context.h"
#include "foxglove/resources/attachments.h"

#include "foxglove/renderer/pass.h"
#include "foxglove/renderer/util.h"


class PassContext {
public:
    PassContext(VkDevice device, 
            FGBufferRegistry& buffers,
            FGTextureRegistry& textures,
            FrameContext* fctx,
            Pass* pass) :
        m_device(device), m_buffers(buffers), m_textures(textures),
        m_fctx(fctx), m_pass(pass) { 
            m_cmd = fctx->get_cmd_buffer(); 
    }

    //TODO: consider removing this
    VkCommandBuffer get_cmd() { return m_cmd; }

    FGTextureHandle get_swapchain_handle() const { 
        return m_fctx->get_swapchain_handle(); 
    }
    
    void bind_compute_pipeline(ComputePipeline* pipeline);
    void dispatch_compute(uint32_t x, uint32_t y, uint32_t z);

    void bind_graphics_pipeline(GraphicsPipeline* pipeline,
        DrawConfig config);
private:
    void fill_attachments(VkRenderingInfo& info, GraphicsPass* pass);

    VkDevice m_device;
    FGBufferRegistry& m_buffers;
    FGTextureRegistry& m_textures;
    
    VkCommandBuffer m_cmd;

    FrameContext* m_fctx;

    Pass* m_pass;
};

