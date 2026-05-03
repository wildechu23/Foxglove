#include "foxglove/core/engine.h"

Engine::Engine(const EngineConfig& config) {
    m_window = std::make_unique<Window>(
        config.window.width,
        config.window.height,
        config.window.title
    );
    m_window->init();

    if(config.enable_input) {
        m_input = std::make_unique<InputManager>(*m_window);
    }

    if(config.enable_input && config.camera.has_value()) {
        m_camera = std::make_unique<Camera>(
            config.camera->fov,
            config.camera->near_plane,
            config.camera->far_plane,
            config.camera->aspect_ratio
        );
    }

    if(config.graphics.has_value()) {
        m_vk_context = std::make_unique<VulkanContext>();
        m_vk_context->init(*m_window);

        m_resources = std::make_unique<ResourceManager>(
            *m_vk_context);

        m_renderer = std::make_unique<Renderer>(
                *m_window, *m_vk_context, *m_resources);

        if(config.graphics->enable_upload) {
            m_upload = std::make_unique<UploadManager>(
                *m_vk_context, *m_resources);
            m_upload->init();
        }
    }
}

Engine::~Engine() {
    vkDeviceWaitIdle(m_vk_context->get_device());
    
    m_resources->cleanup();
    m_upload->cleanup();

    m_renderer->cleanup();
    m_vk_context->cleanup();
}

void Engine::begin_frame() {
    m_window->update();

    if(m_input) m_input->update();
    if(m_upload) m_upload->process_completions();

    static auto last_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    m_delta_time = std::chrono::duration<float>(now - last_time).count();
    last_time = now;
    
    if (m_camera && m_input) {
        m_camera->update(*m_input, m_delta_time);
    }
}

void Engine::draw() {
    if(m_renderer) {
        if(m_upload) m_upload->submit_batch();

        m_renderer->draw();
    }
}

void Engine::end_frame() {}
