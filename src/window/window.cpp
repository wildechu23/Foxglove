#include "foxglove/window/window.h"

Window::Window(uint32_t width, uint32_t height, const std::string& title)
    : width(width), height(height), title(title), m_window(nullptr) {}

Window::~Window() {
    if(m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::init() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

void Window::update() {
    glfwPollEvents();
}

bool Window::should_close() const {
    return glfwWindowShouldClose(m_window);
}
