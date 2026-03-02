#pragma once

#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    Window(uint32_t width, uint32_t height, const std::string& title);
    ~Window();

    void init();
    void update();
    bool should_close() const;
    
    uint32_t get_width() const { return width; }
    uint32_t get_height() const { return height; }

    GLFWwindow* get_window() const { return m_window; }
private:
    uint32_t width, height;
    std::string title;
    GLFWwindow* m_window;
};
