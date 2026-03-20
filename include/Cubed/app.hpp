#pragma once

#include <Cubed/camera.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/input.hpp>
#include <Cubed/renderer.hpp>
#include <Cubed/texture_manager.hpp>
#include <Cubed/window.hpp>

class App {
public:
    App();
    ~App();
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void window_reshape_callback(GLFWwindow* window, int new_width, int new_height);
    static int start_cubed_application(int argc, char** argv);
    
    
private:
    Camera m_camera;
    World m_world;
    Renderer m_renderer{m_camera, m_world};

    Window m_window{m_renderer};
    
    
    GLuint m_texture_array;
    
    TextureManager m_texture_manager;
    
    
    void init();
    
    auto init_camera();
    auto init_texture();
    auto init_world();
    
    void render();
    void run();
    void update();
};