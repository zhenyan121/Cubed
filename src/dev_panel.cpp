#include <Cubed/dev_panel.hpp>
#include <Cubed/app.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/tools/log.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace Cubed {

DevPanel::DevPanel(App& app) :
    m_app(app)
{

}

void DevPanel::init() {
    m_player = &m_app.world().get_player("TestPlayer");
    auto config = Config::get();
    m_config.fov = static_cast<float>(config.val_view("player.fov").value_or(70.0));
    m_config.fullscreen = config.val_view("window.fullscreen").value_or(false);
    m_config.v_sync = config.val_view("window.V-Sync").value_or(true);
    m_config.mouse_sensitivity = static_cast<float>(config.val_view("player.mouse_sensitivity").value_or(0.15));
    m_config.width = config.val_view("window.width").value_or(800);
    m_config.height = config.val_view("window.height").value_or(600);
    m_config.rendering_distance = config.val_view("world.rendering_distance").value_or(24);
    m_theme = config.val_view("devpanel.theme").value_or(0);
    if (m_theme != 1 && m_theme != 0) {
        m_theme = 0;
    }
}

void DevPanel::render() {
    ASSERT_MSG(m_player, "Player Is Null");
    /*
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
    */
    
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("DevPanel");                      
    ImGui::Text("This is a DevPanel to control the game\n"); 
    if (ImGui::BeginTabBar("Bar")) {
        show_settings_tab_item();
        ImGui::EndTabBar();
    }
    ImGui::End(); 
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
}

void DevPanel::show_settings_tab_item() {
    if (ImGui::BeginTabItem("settings")) {
        if(ImGui::SliderFloat("FOV", &m_config.fov, 1.0f, 140.0f)) {
            Config::get().set("player.fov", static_cast<double>(m_config.fov));
            m_app.renderer().hot_reload();
        }
        ImGui::SameLine();
        if (ImGui::Button("default##1")) {
            m_config.fov = NORMAL_FOV;
            Config::get().set("player.fov", static_cast<double>(m_config.fov));
            m_app.renderer().hot_reload();
        }
        if (ImGui::SliderFloat("Sensitivity", &m_config.mouse_sensitivity, 0.01f, 1.0f)) {
            Config::get().set("player.mouse_sensitivity", static_cast<double>(m_config.mouse_sensitivity));
            m_player->hot_reload();
        }
        ImGui::SameLine();
        if (ImGui::Button("default##2")) {
            m_config.mouse_sensitivity = 0.15f;
            Config::get().set("player.mouse_sensitivity", static_cast<double>(m_config.mouse_sensitivity));
            m_player->hot_reload();
        }
        if (ImGui::SliderInt("Distance", &m_config.rendering_distance, 2, 128)) {
            Config::get().set("world.rendering_distance", m_config.rendering_distance);
            m_app.world().hot_reload();
        }
        if (ImGui::Checkbox("Fullscreen", &m_config.fullscreen)) {
            Config::get().set("window.fullscreen", m_config.fullscreen);
            m_app.window().hot_reload();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("V-Sync", &m_config.v_sync)) {
            Config::get().set("window.V-Sync", m_config.v_sync);
            m_app.window().hot_reload();
        }
        constexpr const char* THEMES[] = {"Dark", "Light"};
        if (ImGui::Combo("Theme", &m_theme, THEMES, IM_ARRAYSIZE(THEMES))) {
            if (m_theme == 0) {
                ImGui::StyleColorsDark();
            } else if (m_theme == 1) {
                ImGui::StyleColorsLight();
            }
            Config::get().set("devpanel.theme", m_theme);
        }
        if (ImGui::Button("save")) { 
            Config::get().save_to_file();
        }
        
        ImGui::EndTabItem();
    }
    
}

}