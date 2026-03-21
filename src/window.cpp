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
        LOG::error("glfwinit fail");
        exit(EXIT_FAILURE);        
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    m_window = glfwCreateWindow(800, 600, "Cubed", NULL, NULL);
    glfwMakeContextCurrent(m_window);

    glfwSwapInterval(1);   
    

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //update_viewport();
    
}