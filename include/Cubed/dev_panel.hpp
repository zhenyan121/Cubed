#pragma once

#include <toml++/toml.hpp>

namespace Cubed {

class App;
class Player;
class DevPanel {
    struct ConfigView {
        float fov = 70.0f;
        bool fullscreen = false;
        bool v_sync = true;
        float mouse_sensitivity = 0.15f;
        int width = 800;
        int height = 600;
        int rendering_distance = 24;
        int aniso = 1;
        int max_aniso = 1;
        bool is_enable_aniso = false;
        bool is_support_aniso = true;
        bool is_reload = true;
    };
    struct PlayerProfile {
        int game_mode = 0;
        int gait = 0;
        float pos[3] = {0.0f, 0.0f, 0.0f};
    };
    struct TextEditing {
        bool perlin_seed = false;
    };

public:
    DevPanel(App& app);
    void init();
    void render();

private:
    App& m_app;
    ConfigView m_config;
    Player* m_player;
    PlayerProfile m_player_profile;
    TextEditing m_text_editing;
    bool m_need_save_config = false;
    int m_theme = 0;
    void show_about_table_bar();
    void show_biome_table_bar();
    void show_settings_tab_item();
    void show_world_tab_item();
    void show_player_tab_item();
    void show_items_tab_item();

    void update_config_view();
    void update_player_profile();
};

} // namespace Cubed