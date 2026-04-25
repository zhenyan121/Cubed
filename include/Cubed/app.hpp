#pragma once

#include <Cubed/camera.hpp>
#include <Cubed/dev_panel.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/input.hpp>
#include <Cubed/renderer.hpp>
#include <Cubed/texture_manager.hpp>
#include <Cubed/window.hpp>
namespace Cubed {

class App {
public:
    App();
    ~App();
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void window_focus_callback(GLFWwindow* window, int focused);
    static void window_reshape_callback(GLFWwindow* window, int new_width, int new_height);
    static void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void cursor_enter_callback(GLFWwindow* window, int entered);
    static int start_cubed_application(int argc, char** argv);
    static unsigned int seed();
    static float delte_time();
    static float get_fps();

    Camera& camera();
    DevPanel& dev_panel();
    Renderer& renderer();
    TextureManager& texture_manager();
    Window& window();
    World& world();


private:
    Camera m_camera;
    TextureManager m_texture_manager;
    World m_world;
    DevPanel m_dev_panel{*this};
    Renderer m_renderer{m_camera, m_world, m_texture_manager, m_dev_panel};

    Window m_window{m_renderer};
    
    inline static double last_time = glfwGetTime();
    inline static double current_time = glfwGetTime();
    inline static double delta_time = 0.0f;
    inline static double fps_time_count = 0.0f;
    inline static int frame_count = 0;
    inline static int fps = 0;
    
    void init();
    
    auto init_camera();
    auto init_texture();
    auto init_world();
    
    void render();
    void run();
    void update();
};

}