#pragma once

#include "foxglove/window/window.h"
#include "foxglove/vulkan/vulkan_context.h"
#include "foxglove/vulkan/swapchain.h"

#include "foxglove/resources/frame_context.h"
#include "foxglove/resources/shader_library.h"

#include "foxglove/renderer/framegraph.h"

#include "foxglove/resources/pipeline_manager.h"

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
private:
    int m_frame_num{0};
    FrameContext& get_current_frame() { return m_frames[m_frame_num % FRAME_OVERLAP]; };
    const uint32_t FRAME_OVERLAP = 3;

    Window& m_window;
    VulkanContext m_ctx;
    Swapchain m_swapchain;

    FrameGraph m_fg;

    ShaderLibrary m_sl;
    PipelineManager m_pm;

    // change to frames when using frame contexts 
    std::vector<FrameContext> m_frames{FRAME_OVERLAP};
};
