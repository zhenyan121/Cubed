#include <Cubed/app.hpp>
#include <Cubed/debug_collector.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/cubed_random.hpp>
#include <Cubed/tools/system_info.hpp>
#include <Cubed/tools/perlin_noise.hpp>

#include <exception>

namespace Cubed {


App::App() {

}

App::~App() {

}
void App::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    if (!app->m_window.is_mouse_enable()) {
        app->m_camera.update_cursor_position_camera(xpos, ypos);
    }
    
}
void App::init() {
    m_window.init();
    Logger::info("Window Init Success");

    glfwSetWindowUserPointer(m_window.get_glfw_window(), this);
    
    glfwSetCursorPosCallback(m_window.get_glfw_window(), cursor_position_callback);
    glfwSetMouseButtonCallback(m_window.get_glfw_window(), mouse_button_callback);
    glfwSetWindowFocusCallback(m_window.get_glfw_window(), window_focus_callback);
    glfwSetWindowSizeCallback(m_window.get_glfw_window(), window_reshape_callback);
    glfwSetKeyCallback(m_window.get_glfw_window(), key_callback);
    glfwSetScrollCallback(m_window.get_glfw_window(), mouse_scroll_callback);
    PerlinNoise::init();
    
    m_renderer.init();
    Logger::info("Renderer Init Success");
    m_window.update_viewport();
    //MapTable::init_map();
    m_texture_manager.init_texture();
    Logger::info("Texture Load Success");
    m_world.init_world();
    Logger::info("World Init Success");

    m_camera.camera_init(&m_world.get_player("TestPlayer"));
    
}

void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    switch(key) {
        case GLFW_KEY_Q:
            if (action == GLFW_PRESS) {
                
                if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            break;
        case GLFW_KEY_ESCAPE:
            if (action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            break;
        case GLFW_KEY_F11:
            if (action == GLFW_PRESS) {
                app->m_window.toggle_fullscreen();
            }
            break;
        case GLFW_KEY_R:
            if (action == GLFW_PRESS) {
                app->m_texture_manager.need_reload();
            }
            break;
        case GLFW_KEY_P:
            if (action == GLFW_PRESS) {
                app->m_window.toggle_mouse_able();
            }
            break;
            
    }

    app->m_world.get_player("TestPlayer").update_player_move_state(key, action);
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS) {
                Input::get_input_state().mouse_state.left = true;
            }
            if (action == GLFW_RELEASE) {
                Input::get_input_state().mouse_state.left = false;
            } 
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                Input::get_input_state().mouse_state.right = true;
            }
            if (action == GLFW_RELEASE) {
                Input::get_input_state().mouse_state.right = false;
            }
            break;
    }
}

void App::window_focus_callback(GLFWwindow* window, int focused) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    if (focused) {
        app->m_camera.reset_camera();
    }
    
}

void App::window_reshape_callback(GLFWwindow* window, int new_width, int new_height) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    app->m_window.update_viewport();
}

void App::mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    auto& player = app->m_world.get_player("TestPlayer");
    player.update_scroll(yoffset);
}

void App::render() {

    m_renderer.render();

    glfwSwapBuffers(m_window.get_glfw_window());

}

void App::run() {
    
    last_time = glfwGetTime();
    while(!glfwWindowShouldClose(m_window.get_glfw_window())) {
        
        update();
        render(); 
        
    }
}
static Gait player_gait = Gait::WALK;
void App::update() {
    glfwPollEvents();
    current_time = glfwGetTime();
    delta_time = current_time - last_time;
    last_time = current_time;
    fps_time_count += delta_time;
    frame_count++;
    if (fps_time_count >= 1.0f) {
        fps = static_cast<int>(frame_count / fps_time_count);
        std::string title = "Cubed FPS: " + std::to_string(fps);
        glfwSetWindowTitle(m_window.get_glfw_window(), title.c_str());
        frame_count = 0;
        fps_time_count = 0.0f;
        DebugCollector::get().report("fps", std::string{"FPS: " + std::to_string(fps)});
        DebugCollector::get().report("rss", std::format("RSS: {}mb", Tools::get_current_rss() / (1024 * 1024)));
    }
    m_texture_manager.update();
    m_world.update(delta_time);
    m_camera.update_move_camera();
    const auto& player= m_world.get_player("TestPlayer");
    if (player_gait != player.get_gait()) {
        player_gait = player.get_gait();
        if (player_gait == Gait::WALK) {
            m_renderer.update_fov(NORMAL_FOV);
        }
        if (player_gait == Gait::RUN) {
            m_renderer.update_fov(NORMAL_FOV + 3.0f);
        }
    }

}

int App::start_cubed_application(int argc, char** argv) {

    App app;

    try {
        app.init();
        Logger::info("Game Init Finish Start Run...");
        app.run();

        return 0;
    } catch (std::exception& e) {
        Logger::error("{}", e.what());
        
    } catch (...) {
        Logger::error("Unkown error");
        
    }
    return 1;
}

float App::delte_time() {
    return delta_time;
}

float App::get_fps() {
    return fps;
}

}