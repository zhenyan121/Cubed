#pragma once

#include <toml++/toml.hpp>

namespace Cubed {

class App;
class Player;
class DevPanel {
    struct ConfigView {
        float fov;
        bool fullscreen;
        bool v_sync;
        float mouse_sensitivity;
        int width;
        int height;
        int rendering_distance;
    };
public:
    DevPanel(App& app);
    void init();
    void render();
    
private:
    App& m_app;
    ConfigView m_config;
    Player* m_player;
    
    bool m_need_save_config = false;
    int m_theme = 0;
    void show_settings_tab_item();
    void show_world_tab_item();
    void show_player_tab_item();
};


}