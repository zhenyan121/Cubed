#pragma once
#include <GLFW/glfw3.h>
namespace Cubed{

class Renderer;
class Window {
public:
    Window(Renderer& renderer);
    ~Window();

    bool is_mouse_enable() const;
    const GLFWwindow* get_glfw_window() const;
    GLFWwindow* get_glfw_window();
    void init();
    void update_viewport();
    // end of frame to reload!
    void hot_reload();

    void toggle_fullscreen();
    void toggle_mouse_able();
    
private:
    bool m_mouse_enable = false;
    float m_aspect;
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    Renderer& m_renderer;
    
};

}