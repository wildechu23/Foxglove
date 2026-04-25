#pragma once

#include "foxglove/window/window.h"
#include "foxglove/vulkan/vulkan_context.h"
#include "foxglove/vulkan/swapchain.h"

#include "foxglove/framegraph/frame_context.h"
#include "foxglove/framegraph/framegraph.h"

#include "foxglove/resources/resource_manager.h"
#include "foxglove/resources/upload_manager.h"
#include "foxglove/resources/shader/shader_library.h"
#include "foxglove/resources/pipeline/pipeline_manager.h"

#include <vector>

class Renderer {
public:
    Renderer(Window& window);
    ~Renderer();

    void init();
    void draw();
    void cleanup();

    FrameGraph& get_fg() { return m_fg; }
    ShaderLibrary& get_sl() { return m_sl; }
    PipelineManager& get_pm() { return m_pm; }
    ResourceManager& get_rm() { return m_rm; }
    UploadManager& get_um() { return m_um; }
private:
    int m_frame_num{0};
    FrameContext& get_current_frame() { return m_frames[m_frame_num % FRAME_OVERLAP]; };
    const uint32_t FRAME_OVERLAP = 3;

    Window& m_window;
    VulkanContext m_ctx;
    Swapchain m_swapchain;

    FrameGraph m_fg;
    ResourceManager m_rm;

    UploadManager m_um;

    ShaderLibrary m_sl;
    PipelineManager m_pm;

    // change to frames when using frame contexts 
    std::vector<FrameContext> m_frames{FRAME_OVERLAP};
};
