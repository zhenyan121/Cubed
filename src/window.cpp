#include <Cubed/renderer.hpp>
#include <Cubed/input.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/window.hpp>

Window::Window(Renderer& renderer) :
    m_renderer(renderer)
{
    
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

const GLFWwindow* Window::get_glfw_window() const {
    return m_window;
}

GLFWwindow* Window::get_glfw_window() {
    return m_window;
}

void Window::update_viewport() {
    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    m_aspect = (float)m_width / (float)m_height;
    glViewport(0, 0, m_width, m_height);
    m_renderer.update_proj_matrix(m_aspect, m_width, m_height) ;

}

void Window::init() {
    if (!glfwInit()) {
        Logger::error("glfwinit fail");
        exit(EXIT_FAILURE);        
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(800, 600, "Cubed", NULL, NULL);
    
    glfwMakeContextCurrent(m_window);

    glfwSwapInterval(1);   
    
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    } else {
        Logger::warn("Don,t support Raw Mouse Motion");
    }

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    glfwSetWindowPos(m_window, static_cast<int>(mode->width / 2.0f) - 400, static_cast<int>(mode->height / 2.0f) - 300);
    //update_viewport();
    
}

void Window::toggle_fullscreen() {
    static int windowed_xpos = 0, windowed_ypos = 0;
    static int windowed_width = 800, windowed_height = 600;

    GLFWmonitor* monitor = glfwGetWindowMonitor(m_window);
    if (monitor != nullptr) {
        glfwSetWindowMonitor(
            m_window, 
            nullptr, 
            windowed_xpos, 
            windowed_ypos, 
            windowed_width, 
            windowed_height, 
            0
        );
    } else {
        glfwGetWindowPos(m_window, &windowed_xpos, &windowed_ypos);
        glfwGetWindowSize(m_window, &windowed_width, &windowed_height);

        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primary);

        glfwSetWindowMonitor(
            m_window, 
            primary, 
            0, 
            0, 
            mode->width, 
            mode->height, 
            GL_DONT_CARE
        );
    }
    update_viewport();
}