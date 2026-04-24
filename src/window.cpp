#include <Cubed/renderer.hpp>
#include <Cubed/input.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/window.hpp>
namespace Cubed {

static int windowed_xpos = 0, windowed_ypos = 0;
static int windowed_width = 800, windowed_height = 600;

Window::Window(Renderer& renderer) :
    m_renderer(renderer)
{
    
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    
    glfwTerminate();
}

bool Window::is_mouse_enable() const {
    return m_mouse_enable;
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
    auto& config = Config::get();

    config.set("window.width", windowed_width);
    config.set("window.height", windowed_height);
}

void Window::init() {
    if (!glfwInit()) {
        Logger::error("glfw init fail");
        exit(EXIT_FAILURE);        
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    auto& config= Config::get();
    m_width = config.get<int>("window.width");
    m_height = config.get<int>("window.height");
    if (config.get<bool>("window.fullscreen")) {
        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
        m_window = glfwCreateWindow(m_width, m_height, "Cubed", primary_monitor, NULL);
    } else {
        m_window = glfwCreateWindow(m_width, m_height, "Cubed", NULL, NULL);
    }
    
    glfwMakeContextCurrent(m_window);
    if (config.get<bool>("window.V-Sync")) {
        glfwSwapInterval(1);   
    } else {
        glfwSwapInterval(0);   
    }
    
    
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

void Window::hot_reload() {
    auto& config= Config::get();
    // V-Sync
    if (config.get<bool>("window.V-Sync")) {
        glfwSwapInterval(1);   
    } else {
        glfwSwapInterval(0);   
    }
    // Window
    windowed_width = config.get<int>("window.width");
    windowed_height = config.get<int>("window.height");

    if (config.get<bool>("window.fullscreen.V-Sync")) {
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

        }
        config.set("window.fullscreen", false);
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
        config.set("window.fullscreen", true);
        
    }
    update_viewport();

}

void Window::toggle_fullscreen() {

    auto& config = Config::get();
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
        
        config.set("window.fullscreen", false);
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
        config.set("window.fullscreen", true);
        
    }
    update_viewport();
}

void Window::toggle_mouse_able() {
    if (m_mouse_enable) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        m_mouse_enable = false;
    } else {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_mouse_enable = true;
    }
}

}