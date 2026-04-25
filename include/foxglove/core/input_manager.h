#pragma once

#include "foxglove/window/window.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <functional>

class InputManager {
public:
    InputManager(Window& window);
    ~InputManager();
    
    void update();
    
    bool is_key_pressed(int key) const;
    bool is_key_released(int key) const;
    bool is_key_held(int key) const;


    bool is_mouse_pressed(int button) const;
    bool is_mouse_released(int button) const;
    bool is_mouse_held(int button) const;

    glm::vec2 get_mouse_position() const { return m_mouse_position; }
    glm::vec2 get_mouse_delta() const { return m_mouse_delta; }
    float get_scroll_delta() const { return m_scroll_delta; }
    
    // Raw input (for high precision mouse movement)
    void set_raw_mouse(bool enabled);
    bool is_raw_mouse_enabled() const { return m_raw_mouse; }
private:
    // GLFW static callbacks
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, 
            int action, int mods);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, 
            int action, int mods);
    static void glfw_cursor_position_callback(GLFWwindow* window, 
            double xpos, double ypos);
    static void glfw_scroll_callback(GLFWwindow* window, 
            double xoffset, double yoffset);

    // Internal state updates
    void update_key_state(int key, int action);
    void update_mouse_state(int button, int action);
    void update_mouse_position(double x, double y);
    void update_scroll(double xoffset, double yoffset);

    struct KeyState {
        bool pressed = false;
        bool released = false;
        bool held = false;
    };
    
    std::unordered_map<int, KeyState> m_keys;
    std::unordered_map<int, KeyState> m_buttons;

    glm::vec2 m_mouse_position{0.0f, 0.0f};
    glm::vec2 m_prev_mouse_position{0.0f, 0.0f};
    glm::vec2 m_mouse_delta{0.0f, 0.0f};
    float m_scroll_delta = 0.0f;
    float m_scroll_accumulator = 0.0f;
    
    bool m_raw_mouse = false;

    GLFWwindow* m_window;

    std::function<void(int, int)> m_key_callback;
    std::function<void(int, int)> m_mouse_button_callback;
    std::function<void(double, double)> m_mouse_move_callback;
    std::function<void(double, double)> m_scroll_callback;
};
