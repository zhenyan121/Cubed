#include "Cubed/app.hpp"

#include "Cubed/config.hpp"
#include "Cubed/debug_collector.hpp"
#include "Cubed/gameplay/player.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/system_info.hpp"

#include <exception>
#include <imgui_impl_glfw.h>

namespace Cubed {

App::App() {}

App::~App() {}
void App::cursor_position_callback(GLFWwindow* window, double xpos,
                                   double ypos) {
    ImGuiIO& io = ImGui::GetIO();

    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));

    ASSERT_MSG(app, "nullptr");
    if (io.WantCaptureMouse && app->m_window.is_mouse_enable()) {
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
        return;
    }
    if (!app->m_window.is_mouse_enable()) {
        app->m_camera.update_cursor_position_camera(xpos, ypos);
    }
}
void App::init() {
    m_window.init();
    m_window.imgui_init();
    Logger::info("Window Init Success");

    glfwSetWindowUserPointer(m_window.get_glfw_window(), this);

    glfwSetCursorPosCallback(m_window.get_glfw_window(),
                             cursor_position_callback);
    glfwSetMouseButtonCallback(m_window.get_glfw_window(),
                               mouse_button_callback);
    glfwSetWindowFocusCallback(m_window.get_glfw_window(),
                               window_focus_callback);
    glfwSetWindowSizeCallback(m_window.get_glfw_window(),
                              window_reshape_callback);
    glfwSetKeyCallback(m_window.get_glfw_window(), key_callback);
    glfwSetScrollCallback(m_window.get_glfw_window(), mouse_scroll_callback);
    glfwSetCursorEnterCallback(m_window.get_glfw_window(),
                               cursor_enter_callback);
    glfwSetCharCallback(m_window.get_glfw_window(), char_callback);
    ChunkGenerator::init();

    m_renderer.init();
    Logger::info("Renderer Init Success");
    m_window.update_viewport();
    // MapTable::init_map();
    m_texture_manager.init_texture();
    Logger::info("Texture Load Success");
    m_world.init_world();
    Logger::info("World Init Success");

    m_camera.camera_init(&m_world.get_player("TestPlayer"));
    m_dev_panel.init();
}

void App::key_callback(GLFWwindow* window, int key, int scancode, int action,
                       int mods) {
    ImGuiIO& io = ImGui::GetIO();

    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    // ImGui_ImplGlfw_CursorEnterCallback(window,
    // !app->m_window.is_mouse_enable());
    if (io.WantCaptureKeyboard && app->m_window.is_mouse_enable()) {
        if ((key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_ESCAPE) &&
            action == GLFW_PRESS) {
            app->m_window.toggle_mouse_able();
            app->m_camera.reset_camera();
            return;
        }
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        return;
    }
    switch (key) {
    case GLFW_KEY_Q:
        if (action == GLFW_PRESS) {
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
    case GLFW_KEY_LEFT_ALT:
        if (action == GLFW_PRESS) {
            app->m_window.toggle_mouse_able();
            app->m_camera.reset_camera();
        }
        break;
    }

    app->m_world.get_player("TestPlayer").update_player_move_state(key, action);
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action,
                                int mods) {
    ImGuiIO& io = ImGui::GetIO();
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    if (io.WantCaptureMouse && app->m_window.is_mouse_enable()) {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
        return;
    }
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
        if (action == GLFW_PRESS) {
            if (app->m_window.is_mouse_enable()) {
                app->m_window.toggle_mouse_able();
                app->m_camera.reset_camera();
                break;
                ;
            }
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
    ImGuiIO& io = ImGui::GetIO();
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    if (io.WantCaptureMouse && app->m_window.is_mouse_enable()) {
        ImGui_ImplGlfw_WindowFocusCallback(window, focused);
        return;
    }
    if (focused) {
        app->m_camera.reset_camera();
    }
}

void App::window_reshape_callback(GLFWwindow* window, int new_width,
                                  int new_height) {

    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    app->m_window.update_viewport();
}

void App::mouse_scroll_callback(GLFWwindow* window, double xoffset,
                                double yoffset) {
    ImGuiIO& io = ImGui::GetIO();

    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    if (io.WantCaptureMouse && app->m_window.is_mouse_enable()) {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
        return;
    }
    auto& player = app->m_world.get_player("TestPlayer");
    player.update_scroll(yoffset);
}

void App::cursor_enter_callback(GLFWwindow* window, int entered) {
    ImGuiIO& io = ImGui::GetIO();
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    if (io.WantCaptureMouse && app->m_window.is_mouse_enable()) {
        ImGui_ImplGlfw_CursorEnterCallback(window, entered);
        return;
    }
}

void App::char_callback(GLFWwindow* window, unsigned int c) {
    ImGuiIO& io = ImGui::GetIO();
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    ASSERT_MSG(app, "nullptr");
    if (io.WantCaptureKeyboard && app->m_window.is_mouse_enable()) {
        ImGui_ImplGlfw_CharCallback(window, c);
    }
}

void App::render() {

    if (glfwGetWindowAttrib(m_window.get_glfw_window(), GLFW_ICONIFIED) != 0) {
        ImGui_ImplGlfw_Sleep(10);
        return;
    }
    m_renderer.render();

    glfwSwapBuffers(m_window.get_glfw_window());
}

void App::run() {

    last_time = glfwGetTime();
    while (!glfwWindowShouldClose(m_window.get_glfw_window())) {

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
        DebugCollector::get().report(
            "fps", std::string{"FPS: " + std::to_string(fps)});
        DebugCollector::get().report(
            "rss",
            std::format("RSS: {}mb", Tools::get_current_rss() / (1024 * 1024)));
    }
    m_texture_manager.update();
    m_world.update(delta_time);
    m_camera.update_move_camera();
    const auto& player = m_world.get_player("TestPlayer");
    if (player_gait != player.get_gait()) {
        player_gait = player.get_gait();
        float fov = static_cast<float>(Config::get().get<double>("player.fov"));
        if (player_gait == Gait::WALK) {
            m_renderer.update_fov(fov);
        }
        if (player_gait == Gait::RUN) {
            m_renderer.update_fov(fov + 5.0f);
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

float App::delte_time() { return delta_time; }

float App::get_fps() { return fps; }

Camera& App::camera() { return m_camera; }
DevPanel& App::dev_panel() { return m_dev_panel; }
Renderer& App::renderer() { return m_renderer; }
TextureManager& App::texture_manager() { return m_texture_manager; }
Window& App::window() { return m_window; }
World& App::world() { return m_world; }

} // namespace Cubed