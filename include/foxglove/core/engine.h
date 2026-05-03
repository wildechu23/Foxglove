#pragma once

#include "foxglove/window/window.h"
#include "foxglove/core/camera.h"
#include "foxglove/core/input_manager.h"
#include "foxglove/renderer/renderer.h"
#include "foxglove/resources/resource_manager.h"
#include "foxglove/resources/upload_manager.h"

#include <cstdint>

struct EngineConfig {
    struct WindowConfig {
        uint32_t width = 1280;
        uint32_t height = 720;
        std::string title = "Foxglove Application";
    };

    WindowConfig window;
    bool enable_input = false;

    struct CameraConfig {
        float fov = 70.0f;
        float near_plane = 0.1f;
        float far_plane = 1000.0f;
        float aspect_ratio = 1280.f / 720.f;
    };
    std::optional<CameraConfig> camera;

    struct GraphicsConfig {
        bool enable_resources = true;
        bool enable_upload = true;
    };
    std::optional<GraphicsConfig> graphics;
};

class Engine {
public:
    explicit Engine(const EngineConfig& config = {});
    ~Engine();
        
    // no copying
    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;

    Window* window() { return m_window.get(); }
    InputManager* input() { return m_input.get(); }
    Camera* camera() { return m_camera.get(); }

    Renderer* renderer() { return m_renderer.get(); }
    ResourceManager* resources() { return m_resources.get(); }
    UploadManager* upload_manager() { return m_upload.get(); }

    void begin_frame();
    void end_frame();
    void draw();
    
    void run(std::function<void(float dt)> update = nullptr);
private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<InputManager> m_input;
    std::unique_ptr<Camera> m_camera;
    
    std::unique_ptr<VulkanContext> m_vk_context;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<ResourceManager> m_resources;
    std::unique_ptr<UploadManager> m_upload;
    
    EngineConfig m_config;
    std::chrono::high_resolution_clock::time_point m_last_frame_time;
    float m_delta_time = 0.0f;
};
