#include "Cubed/dev_panel.hpp"

#include "Cubed/app.hpp"
#include "Cubed/config.hpp"
#include "Cubed/gameplay/player.hpp"
#include "Cubed/map_table.hpp"
#include "Cubed/tools/log.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace Cubed {

static constexpr const char* THEMES[] = {"Dark", "Light"};
static constexpr const char* GAITS[] = {"Walk", "Run"};
static constexpr const char* GAME_MODES[] = {"Creative", "Spectator"};
static char perlin_noise_input_buffer[64];

constexpr float TEMP_MIN = 0.0f;
constexpr float TEMP_MAX = 1.0f;
constexpr float HUMID_MIN = 0.0f;
constexpr float HUMID_MAX = 1.0f;
constexpr float FREQ1_MIN = 0.001f;
constexpr float FREQ1_MAX = 0.020f;
constexpr float FREQ2_MIN = 0.005f;
constexpr float FREQ2_MAX = 0.050f;
constexpr float FREQ3_MIN = 0.005f;
constexpr float FREQ3_MAX = 0.080f;
constexpr int HEIGHT_BASE_MIN = 50;
constexpr int HEIGHT_BASE_MAX = 175;
constexpr int AMPLITUDE_MIN = 2;
constexpr int AMPLITUDE_MAX = 80;
constexpr float TREE_FREQ_MIM = 0.001f;
constexpr float TREE_FREQ_MAX = 0.3f;

static int filter_unsigned(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        char c = data->EventChar;
        if (c < '0' || c > '9') {
            return 1;
        }
    }
    return 0;
}

DevPanel::DevPanel(App& app) : m_app(app) {}

void DevPanel::init() {
    m_player = &m_app.world().get_player("TestPlayer");
    update_config_view();
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
    if (ImGui::BeginTabBar("Menu")) {
        show_settings_tab_item();
        show_world_tab_item();
        show_player_tab_item();
        show_items_tab_item();
        show_about_table_bar();

        ImGui::EndTabBar();
    }
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DevPanel::show_about_table_bar() {
    if (ImGui::BeginTabItem("about")) {
        ImGui::Text(
            "Cubed - A cube game like Minecraft, using C++ and OpenGL.");
        ImGui::Text("Author: zhenyan121");
        ImGui::Separator();
        ImGui::Text("Libraries Used");
        ImGui::Text("glad");
        ImGui::Text("GLFW");
        ImGui::Text("SOIL2");
        ImGui::Text("GLM");
        ImGui::Text("FreeType");
        ImGui::Text("toml++");
        ImGui::Text("Dear ImGui");
        ImGui::Separator();
        ImGui::Text("Special Thanks");
        ImGui::Text("TANGERIME");
        ImGui::Text("SkyOnPole");
        ImGui::Text("free_w_cloud");
        ImGui::Text("Last but not least, thanks to you");
        ImGui::EndTabItem();
    }
}

void DevPanel::show_biome_table_bar() {
    ImGui::Text("Biome");
    if (ImGui::BeginTabBar("Biome")) {
        if (ImGui::BeginTabItem("Plain")) {
            ImGui::SliderFloat("MinTemp##plain", &plain_params().temp.first,
                               TEMP_MIN, TEMP_MAX);
            ImGui::SliderFloat("MaxTemp##plain", &plain_params().temp.second,
                               TEMP_MIN, TEMP_MAX);
            ImGui::SliderFloat("MinHumid##plain", &plain_params().humid.first,
                               HUMID_MIN, HUMID_MAX);
            ImGui::SliderFloat("MaxHumid##plain", &plain_params().humid.second,
                               HUMID_MIN, HUMID_MAX);
            ImGui::SliderFloat("Freq One##plain",
                               &plain_params().frequencies[0], FREQ1_MIN,
                               FREQ1_MAX);
            ImGui::SliderFloat("Freq Two##plain",
                               &plain_params().frequencies[1], FREQ2_MIN,
                               FREQ2_MAX);
            ImGui::SliderFloat("Freq Three##plain",
                               &plain_params().frequencies[2], FREQ3_MIN,
                               FREQ3_MAX);
            ImGui::SliderInt("Base Y##plain",
                             &plain_params().height_range.base_y,
                             HEIGHT_BASE_MIN, HEIGHT_BASE_MAX);
            ImGui::SliderInt("Amplitude##plain",
                             &plain_params().height_range.amplitude,
                             AMPLITUDE_MIN, AMPLITUDE_MAX);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Forest")) {
            ImGui::SliderFloat("MinTemp##forest", &forest_params().temp.first,
                               TEMP_MIN, TEMP_MAX);
            ImGui::SliderFloat("MaxTemp##forest", &forest_params().temp.second,
                               TEMP_MIN, TEMP_MAX);
            ImGui::SliderFloat("MinHumid##forest", &forest_params().humid.first,
                               HUMID_MIN, HUMID_MAX);
            ImGui::SliderFloat("MaxHumid##forest",
                               &forest_params().humid.second, HUMID_MIN,
                               HUMID_MAX);
            ImGui::SliderFloat("Freq One##forest",
                               &forest_params().frequencies[0], FREQ1_MIN,
                               FREQ1_MAX);
            ImGui::SliderFloat("Freq Two##forest",
                               &forest_params().frequencies[1], FREQ2_MIN,
                               FREQ2_MAX);
            ImGui::SliderFloat("Freq Three##forest",
                               &forest_params().frequencies[2], FREQ3_MIN,
                               FREQ3_MAX);
            ImGui::SliderInt("Base Y##forest",
                             &forest_params().height_range.base_y,
                             HEIGHT_BASE_MIN, HEIGHT_BASE_MAX);
            ImGui::SliderInt("Amplitude##forest",
                             &forest_params().height_range.amplitude,
                             AMPLITUDE_MIN, AMPLITUDE_MAX);
            ImGui::SliderFloat("Tree Freq##forest",
                               &forest_params().tree_frequency, TREE_FREQ_MIM,
                               TREE_FREQ_MAX);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Desert")) {
            ImGui::SliderFloat("MinTemp##desert", &desert_params().temp.first,
                               TEMP_MIN, TEMP_MAX);
            ImGui::SliderFloat("MaxTemp##desert", &desert_params().temp.second,
                               TEMP_MIN, TEMP_MAX);
            ImGui::SliderFloat("MinHumid##desert", &desert_params().humid.first,
                               HUMID_MIN, HUMID_MAX);
            ImGui::SliderFloat("MaxHumid##desert",
                               &desert_params().humid.second, HUMID_MIN,
                               HUMID_MAX);
            ImGui::SliderFloat("Freq One##desert",
                               &desert_params().frequencies[0], FREQ1_MIN,
                               FREQ1_MAX);
            ImGui::SliderFloat("Freq Two##desert",
                               &desert_params().frequencies[1], FREQ2_MIN,
                               FREQ2_MAX);
            ImGui::SliderFloat("Freq Three##desert",
                               &desert_params().frequencies[2], FREQ3_MIN,
                               FREQ3_MAX);
            ImGui::SliderInt("Base Y##desert",
                             &desert_params().height_range.base_y,
                             HEIGHT_BASE_MIN, HEIGHT_BASE_MAX);
            ImGui::SliderInt("Amplitude##desert",
                             &desert_params().height_range.amplitude,
                             AMPLITUDE_MIN, AMPLITUDE_MAX);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Mountain")) {
            ImGui::SliderFloat("MinTemp##mountain",
                               &mountain_params().temp.first, TEMP_MIN,
                               TEMP_MAX);
            ImGui::SliderFloat("MaxTemp##mountain",
                               &mountain_params().temp.second, TEMP_MIN,
                               TEMP_MAX);
            ImGui::SliderFloat("MinHumid##mountain",
                               &mountain_params().humid.first, HUMID_MIN,
                               HUMID_MAX);
            ImGui::SliderFloat("MaxHumid##mountain",
                               &mountain_params().humid.second, HUMID_MIN,
                               HUMID_MAX);
            ImGui::SliderFloat("Freq One##mountain",
                               &mountain_params().frequencies[0], FREQ1_MIN,
                               FREQ1_MAX);
            ImGui::SliderFloat("Freq Two##mountain",
                               &mountain_params().frequencies[1], FREQ2_MIN,
                               FREQ2_MAX);
            ImGui::SliderFloat("Freq Three##mountain",
                               &mountain_params().frequencies[2], FREQ3_MIN,
                               FREQ3_MAX);
            ImGui::SliderInt("Base Y##mountain",
                             &mountain_params().height_range.base_y,
                             HEIGHT_BASE_MIN, HEIGHT_BASE_MAX);
            ImGui::SliderInt("Amplitude##mountain",
                             &mountain_params().height_range.amplitude,
                             AMPLITUDE_MIN, AMPLITUDE_MAX);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("River")) {
            ImGui::SliderFloat("MinTemp##river", &river_params().temp.first,
                               TEMP_MIN, TEMP_MAX);
            ImGui::SliderFloat("MaxTemp##river", &river_params().temp.second,
                               TEMP_MIN, TEMP_MAX);
            ImGui::SliderFloat("MinHumid##river", &river_params().humid.first,
                               HUMID_MIN, HUMID_MAX);
            ImGui::SliderFloat("MaxHumid##river", &river_params().humid.second,
                               HUMID_MIN, HUMID_MAX);
            ImGui::SliderFloat("Freq One##river",
                               &river_params().frequencies[0], FREQ1_MIN,
                               FREQ1_MAX);
            ImGui::SliderFloat("Freq Two##river",
                               &river_params().frequencies[1], FREQ2_MIN,
                               FREQ2_MAX);
            ImGui::SliderFloat("Freq Three##river",
                               &river_params().frequencies[2], FREQ3_MIN,
                               FREQ3_MAX);
            ImGui::SliderInt("Base Y##river",
                             &river_params().height_range.base_y,
                             HEIGHT_BASE_MIN, HEIGHT_BASE_MAX);
            ImGui::SliderInt("Amplitude##river",
                             &river_params().height_range.amplitude,
                             AMPLITUDE_MIN, AMPLITUDE_MAX);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void DevPanel::show_settings_tab_item() {
    if (ImGui::BeginTabItem("settings")) {
        if (ImGui::SliderFloat("FOV", &m_config.fov, 1.0f, 140.0f)) {
            Config::get().set("player.fov", static_cast<double>(m_config.fov));
            m_app.renderer().hot_reload();
        }
        ImGui::SameLine();
        if (ImGui::Button("default##1")) {
            m_config.fov = DEFAULT_FOV;
            Config::get().set("player.fov", static_cast<double>(m_config.fov));
            m_app.renderer().hot_reload();
        }
        if (ImGui::SliderFloat("Sensitivity", &m_config.mouse_sensitivity,
                               0.01f, 1.0f)) {
            Config::get().set("player.mouse_sensitivity",
                              static_cast<double>(m_config.mouse_sensitivity));
            m_player->hot_reload();
        }
        ImGui::SameLine();
        if (ImGui::Button("default##2")) {
            m_config.mouse_sensitivity = 0.15f;
            Config::get().set("player.mouse_sensitivity",
                              static_cast<double>(m_config.mouse_sensitivity));
            m_player->hot_reload();
        }
        if (ImGui::SliderInt("Distance", &m_config.rendering_distance, 2,
                             128)) {
            Config::get().set("world.rendering_distance",
                              m_config.rendering_distance);
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
        if (ImGui::Checkbox("Aniso", &m_config.is_enable_aniso)) {
            m_config.is_reload = false;
            if (m_config.is_enable_aniso) {
                m_config.max_aniso = m_app.texture_manager().max_aniso();
                if (m_config.max_aniso < 2) {
                    m_config.is_support_aniso = false;
                } else {
                    m_config.aniso = 2;
                }
            } else {
                m_config.aniso = 1;
            }
        }
        if (m_config.is_enable_aniso) {
            ImGui::SameLine();
            if (!m_config.is_support_aniso) {
                ImGui::Text("Not Support\n");
            } else {
                if (ImGui::SliderInt("##aniso", &m_config.aniso, 2,
                                     m_config.max_aniso)) {
                    m_config.is_reload = false;
                    int log =
                        static_cast<int>(std::log2(m_config.aniso) + 0.5f);
                    m_config.aniso = static_cast<int>(std::pow(2, log));
                    if (m_config.aniso < 2) {
                        m_config.aniso = 2;
                    }
                    if (m_config.aniso > m_config.max_aniso) {
                        m_config.aniso = m_config.max_aniso;
                    }
                }
            }
        }
        if (ImGui::Button("ReloadTexture")) {
            Config::get().set("texture.aniso", m_config.aniso);
            m_app.texture_manager().hot_reload();
            m_config.is_reload = true;
        }
        if (!m_config.is_reload) {
            ImGui::SameLine();
            ImGui::Text("Your need to click this button to apply config\n");
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
            if (ImGui::InputText("Perlin Noise Seed", perlin_noise_input_buffer,
                                 sizeof(perlin_noise_input_buffer),
                                 ImGuiInputTextFlags_CallbackCharFilter |
                                     ImGuiInputTextFlags_EnterReturnsTrue,
                                 filter_unsigned)) {
                ChunkGenerator::seed(static_cast<unsigned int>(
                    std::strtoul(perlin_noise_input_buffer, nullptr, 10)));
                m_text_editing.perlin_seed = false;
                m_player->set_player_pos({0.0f, 255.0f, 0.0f});
                m_app.world().rebuild_world();
            }
        }
        if (!m_text_editing.perlin_seed) {
            ImGui::Text("ChunkGenerator Seed: %u", ChunkGenerator::seed());
            if (ImGui::IsItemClicked()) {
                m_text_editing.perlin_seed = true;
            }
        }
        static int rendering_distance = m_app.world().rendering_distance();
        if (ImGui::SliderInt("Render Distance", &rendering_distance, 2, 128)) {
            m_app.world().rendering_distance(rendering_distance);
        }
        if (ImGui::Button("Rebuild World")) {
            m_app.world().rebuild_world();
        }
        ImGui::SameLine();
        if (ImGui::Button("Request Chunk Build")) {
            m_app.world().need_gen();
        }
        ImGui::SameLine();
        if (ImGui::Button("Spawn Point")) {
            m_player->set_player_pos({0.0f, 255.0f, 0.0f});
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Gen Thread", &m_gen_thread_running)) {
            if (m_gen_thread_running) {
                m_app.world().start_gen_thread();
            } else {
                m_app.world().stop_gen_thread();
            }
        }
        ImGui::Text("Chunk Build Progress\n");
        ImGui::ProgressBar(m_app.world().chunk_gen_fraction());
        show_biome_table_bar();
        ImGui::EndTabItem();
    }
}

void DevPanel::show_player_tab_item() {
    if (!m_player) {
        Logger::error("Player is Nullptr");
        return;
    }
    if (ImGui::BeginTabItem("player")) {
        if (ImGui::Combo("GameMode", &m_player_profile.game_mode, GAME_MODES,
                         IM_ARRAYSIZE(GAME_MODES))) {
            if (m_player_profile.game_mode == 0) {
                m_player->change_mode(GameMode::CREATIVE);
            } else if (m_player_profile.game_mode == 1) {
                m_player->change_mode(GameMode::SPECTATOR);
            } else {
                ASSERT_MSG(false, "Unknown GameMode");
            }
        }
        if (m_player->game_mode() == GameMode::CREATIVE) {
            if (ImGui::Combo("Gait", &m_player_profile.gait, GAITS,
                             IM_ARRAYSIZE(GAITS))) {
                if (m_player_profile.gait == 0) {
                    m_player->gait() = Gait::WALK;
                } else if (m_player_profile.gait == 1) {
                    m_player->gait() = Gait::RUN;
                } else {
                    ASSERT_MSG(false, "Unknown Gait");
                }
            }
        }
        ImGui::DragFloat3("##player_pos", m_player_profile.pos);
        ImGui::SameLine();
        if (ImGui::Button("TP")) {
            m_player->set_player_pos({m_player_profile.pos[0],
                                      m_player_profile.pos[1],
                                      m_player_profile.pos[2]});
        }
        ImGui::SliderFloat("Acceleration", &m_player->acceleration(), 1.0f,
                           200.0f);
        ImGui::SliderFloat("Deceleration", &m_player->deceleration(), 1.0f,
                           200.0f);
        if (m_player->game_mode() == GameMode::CREATIVE) {
            m_player_profile.game_mode = 0;
            ImGui::SliderFloat("MaxWalkSpeed", &m_player->max_walk_speed(),
                               1.0f, 200.0f);
            ImGui::SliderFloat("MaxRunSpeed", &m_player->max_run_speed(), 1.0f,
                               500.0f);
            ImGui::SliderFloat("G", &m_player->g(), 1.0f, 200.0f);

        } else if (m_player->game_mode() == GameMode::SPECTATOR) {
            m_player_profile.game_mode = 1;
            ImGui::SliderFloat("MaxSpeed", &m_player->max_speed(), 1.0f,
                               500.0f);
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

void DevPanel::show_items_tab_item() {
    auto& textures = m_app.texture_manager().item_textures();
    auto& names = MapTable::item_map();
    if (ImGui::BeginTabItem("item")) {
        ImGui::Text("Place Block ");
        ImGui::SameLine();
        ImGui::Image(static_cast<ImTextureID>(static_cast<intptr_t>(
                         textures[m_player->place_block()])),
                     ImVec2{48, 48});
        for (size_t i = 1; i < textures.size(); i++) {
            if (ImGui::ImageButton(("##item" + std::to_string(i)).c_str(),
                                   static_cast<ImTextureID>(
                                       static_cast<intptr_t>(textures[i])),
                                   ImVec2{48, 48})) {
                m_player->set_place_block(i);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", names[i].c_str());
                ImGui::EndTooltip();
            }
            if (i % 10 != 0) {
                ImGui::SameLine();
            }
        }
        ImGui::EndTabItem();
    }
}

void DevPanel::update_config_view() {
    auto config = Config::get();
    m_config.fov =
        static_cast<float>(config.val_view("player.fov").value_or(70.0));
    m_config.fullscreen = config.val_view("window.fullscreen").value_or(false);
    m_config.v_sync = config.val_view("window.V-Sync").value_or(true);
    m_config.mouse_sensitivity = static_cast<float>(
        config.val_view("player.mouse_sensitivity").value_or(0.15));
    m_config.width = config.val_view("window.width").value_or(800);
    m_config.height = config.val_view("window.height").value_or(600);
    m_config.rendering_distance =
        config.val_view("world.rendering_distance").value_or(24);
    m_theme = config.val_view("devpanel.theme").value_or(0);
    if (m_theme != 1 && m_theme != 0) {
        m_theme = 0;
    }
    m_config.aniso = config.val_view("texture.aniso").value_or(1);
    m_config.max_aniso = m_app.texture_manager().max_aniso();
    if (m_config.aniso <= 1) {
        m_config.is_enable_aniso = false;
    } else {
        m_config.is_enable_aniso = true;
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

} // namespace Cubed