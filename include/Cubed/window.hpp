#pragma once
#include <GLFW/glfw3.h>
class Renderer;
class Window {
public:
    Window(Renderer& renderer);
    ~Window();

    const GLFWwindow* get_glfw_window() const;
    GLFWwindow* get_glfw_window();
    void init();
    void update_viewport();
    
private:
    float m_aspect;
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    Renderer& m_renderer;
    
};