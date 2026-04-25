#include <Cubed/dev_panel.hpp>
#include <Cubed/app.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/perlin_noise.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace Cubed {

static constexpr const char* THEMES[] = {"Dark", "Light"};
static constexpr const char* GAITS[] = {"Walk", "Run"};
static constexpr const char* GAME_MODES[] = {"Creative", "Spectator"};
static char perlin_noise_input_buffer[64];

static int filter_unsigned(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        char c = data->EventChar;
        if (c < '0' || c > '9') {
            return 1;
        }
    }
    return 0;
}

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
    update_player_profile();
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
        show_world_tab_item();
        show_player_tab_item();
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
            m_config.fov = DEFAULT_FOV;
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

void DevPanel::show_world_tab_item() {
    if (ImGui::BeginTabItem("world")) {
        if (m_text_editing.perlin_seed) {
            if (ImGui::InputText("Perlin Noise Seed", perlin_noise_input_buffer, sizeof(perlin_noise_input_buffer),
                    ImGuiInputTextFlags_CallbackCharFilter |
                    ImGuiInputTextFlags_EnterReturnsTrue,
                    filter_unsigned)) 
            {
                PerlinNoise::seed(static_cast<unsigned int>(std::strtoul(perlin_noise_input_buffer, nullptr, 10)));   
                m_text_editing.perlin_seed = false;
            }
        }
        if (!m_text_editing.perlin_seed) {
            ImGui::Text("Perlin Noise Seed: %u", PerlinNoise::seed());
            if (ImGui::IsItemClicked()) {
                m_text_editing.perlin_seed = true;
            }
        }
        if (ImGui::Button("Rebuild World")) {
            m_app.world().rebuild_world();
        }
        ImGui::EndTabItem();
    }
}

void DevPanel::show_player_tab_item() {
    if (!m_player) {
        Logger::error("Player is Nullptr");
        return;
    }
    if (ImGui::BeginTabItem("player")) {
        if (ImGui::Combo("GameMode", &m_player_profile.game_mode, GAME_MODES, IM_ARRAYSIZE(GAME_MODES))) {
            if (m_player_profile.game_mode == 0) {
                m_player->change_mode(GameMode::CREATIVE);
            } else if (m_player_profile.game_mode == 1) {
                m_player->change_mode(GameMode::SPECTATOR);
            } else {
                ASSERT_MSG(false, "Unknown GameMode");
            }
        }
        if (m_player->game_mode() == GameMode::CREATIVE) {
            if (ImGui::Combo("Gait", &m_player_profile.gait, GAITS, IM_ARRAYSIZE(GAITS))) {
            if (m_player_profile.gait == 0) {
                m_player->gait() = Gait::WALK;
            } else if (m_player_profile.gait == 1) {
                m_player->gait() = Gait::RUN;
            } else {
                ASSERT_MSG(false, "Unknown Gait");
            }
        }
        }
        ImGui::SliderFloat("Acceleration", &m_player->acceleration(), 1.0f, 200.0f);
        ImGui::SliderFloat("Deceleration", &m_player->deceleration(), 1.0f, 200.0f);
        if (m_player->game_mode() == GameMode::CREATIVE) {
            m_player_profile.game_mode = 0;
            ImGui::SliderFloat("MaxWalkSpeed", &m_player->max_walk_speed(), 1.0f, 200.0f);
            ImGui::SliderFloat("MaxRunSpeed", &m_player->max_run_speed(), 1.0f, 500.0f);
            ImGui::SliderFloat("G", &m_player->g(), 1.0f, 200.0f);
            
        } else if (m_player->game_mode() == GameMode::SPECTATOR) {
            m_player_profile.game_mode = 1;
            ImGui::SliderFloat("MaxSpeed", &m_player->max_speed(), 1.0f, 500.0f);
        }
        if (ImGui::Button("reset")) {
                m_player->max_walk_speed() = DEFAULT_MAX_WALK_SPEED;
                m_player->max_run_speed() = DEFAULT_MAX_RUN_SPEED;
                m_player->acceleration() = DEFAULT_ACCELERATION;
                m_player->deceleration() = DEFAULT_DECELERATION;
                m_player->g() = DEFAULT_G;
                m_player->change_mode(GameMode::CREATIVE);
                m_player->gait() = Gait::WALK;
                m_player_profile.game_mode = 0;
                m_player_profile.gait = 0;
            }
        if (m_player->gait() == Gait::WALK) {
            m_player_profile.gait = 0;
        } else {
            m_player_profile.gait = 1;
        }
        
        
        ImGui::EndTabItem();
    }
}

void DevPanel::update_player_profile() {
    if (!m_player) {
        Logger::error("Player is Nullptr");
        ASSERT(false);
        return;
    }
    m_player_profile.gait = std::to_underlying(m_player->gait());
    m_player_profile.game_mode = std::to_underlying(m_player->game_mode());
}

}