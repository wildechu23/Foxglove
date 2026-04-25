#include "foxglove/core/input_manager.h"

#include <iostream>

InputManager::InputManager(Window& in_window) {
    GLFWwindow* window = in_window.get_window();
    m_window = window;

    glfwSetWindowUserPointer(window, this);
    
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(window, glfw_cursor_position_callback);
    glfwSetScrollCallback(window, glfw_scroll_callback);
    
    // Get initial mouse position
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    m_mouse_position = glm::vec2(x, y);
    m_prev_mouse_position = m_mouse_position;
}

InputManager::~InputManager() {
    glfwSetKeyCallback(m_window, nullptr);
    glfwSetMouseButtonCallback(m_window, nullptr);
    glfwSetCursorPosCallback(m_window, nullptr);
    glfwSetScrollCallback(m_window, nullptr);
}

void InputManager::update() {
    // Reset per-frame states
    for (auto& [key, state] : m_keys) {
        state.pressed = false;
        state.released = false;
    }
    
    for (auto& [button, state] : m_buttons) {
        state.pressed = false;
        state.released = false;
    }
    
    // Compute mouse delta
    m_mouse_delta = m_mouse_position - m_prev_mouse_position;
    m_prev_mouse_position = m_mouse_position;
    
    // Apply scroll delta
    m_scroll_delta = m_scroll_accumulator;
    m_scroll_accumulator = 0.0f;
}

bool InputManager::is_key_pressed(int key) const {
    auto it = m_keys.find(key);
    return it != m_keys.end() && it->second.pressed;
}

bool InputManager::is_key_released(int key) const {
    auto it = m_keys.find(key);
    return it != m_keys.end() && it->second.released;
}

bool InputManager::is_key_held(int key) const {
    auto it = m_keys.find(key);
    return it != m_keys.end() && it->second.held;
}

bool InputManager::is_mouse_pressed(int button) const {
    auto it = m_buttons.find(button);
    return it != m_buttons.end() && it->second.pressed;
}

bool InputManager::is_mouse_released(int button) const {
    auto it = m_buttons.find(button);
    return it != m_buttons.end() && it->second.released;
}

bool InputManager::is_mouse_held(int button) const {
    auto it = m_buttons.find(button);
    return it != m_buttons.end() && it->second.held;
}

void InputManager::set_raw_mouse(bool enabled) {
    m_raw_mouse = enabled;
    
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, enabled ? GLFW_TRUE : GLFW_FALSE);
    } else if (enabled) {
        std::cerr << "Warning: Raw mouse motion not supported on this platform" << std::endl;
    }
}

void InputManager::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    InputManager* manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!manager) return;
    
    manager->update_key_state(key, action);
    
    if (manager->m_key_callback) {
        manager->m_key_callback(key, action);
    }
}

void InputManager::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    InputManager* manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!manager) return;
    
    manager->update_mouse_state(button, action);
    
    if (manager->m_mouse_button_callback) {
        manager->m_mouse_button_callback(button, action);
    }
}

void InputManager::glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    InputManager* manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!manager) return;
    
    manager->update_mouse_position(xpos, ypos);
    
    if (manager->m_mouse_move_callback) {
        manager->m_mouse_move_callback(xpos, ypos);
    }
}

void InputManager::glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    InputManager* manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!manager) return;
    
    manager->update_scroll(xoffset, yoffset);
    
    if (manager->m_scroll_callback) {
        manager->m_scroll_callback(xoffset, yoffset);
    }
}

void InputManager::update_key_state(int key, int action) {
    KeyState& state = m_keys[key];

    switch (action) {
        case GLFW_PRESS:
            state.pressed = true;
            state.held = true;
            state.released = false;
            break;
        case GLFW_RELEASE:
            state.pressed = false;
            state.held = false;
            state.released = true;
            break;
        case GLFW_REPEAT:
            state.pressed = true;
            state.released = false;
            // held stays the same
            break;
    }
}

void InputManager::update_mouse_state(int button, int action) {
    KeyState& state = m_buttons[button];

    if (action == GLFW_PRESS) {
        state.pressed = true;
        state.held = true;
        state.released = false;
    } else {
        state.pressed = false;
        state.held = false;
        state.released = true;
    }
}

void InputManager::update_mouse_position(double x, double y) {
    m_mouse_position = glm::vec2(x, y);
}

void InputManager::update_scroll(double xoffset, double yoffset) {
    m_scroll_accumulator += yoffset;
}
