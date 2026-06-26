#pragma once
#include "Cubed/gameplay/block.hpp"

#include <glad/glad.h>

namespace Cubed {

class TextureManager {
private:
    bool m_need_reload = false;
    bool m_init = false;
    GLuint m_block_status_array = 0;
    GLuint m_texture_array = 0;
    GLuint m_cross_plane_array = 0;
    GLuint m_ui_array = 0;
    GLuint m_normal_texture_array = 0;
    GLfloat m_max_aniso = 0.0f;

    int m_aniso = 1;

    std::vector<GLuint> m_item_textures;

    void load_block_status(unsigned status_id);
    void load_block_texture(unsigned block_id);
    void load_block_item_texture(unsigned id);
    void load_cross_plane_texture(unsigned id);
    void load_ui_texture(unsigned id);
    void load_pbr_texture(unsigned id);
    void init_item();
    void init_block();
    void init_ui();
    void init_block_status();
    void hot_reload();

public:
    TextureManager();
    ~TextureManager();

    void delet_texture();
    GLuint get_block_status_array() const;
    GLuint get_texture_array() const;
    GLuint get_cross_plane_array() const;
    GLuint get_ui_array() const;
    GLuint get_pbr_texture() const;
    const std::vector<GLuint>& item_textures() const;
    // Must call after MapTable::init_map() and glfwMakeContextCurrent(window);
    void init_texture();

    void need_reload();
    void update();
    int max_aniso() const;
};

} // namespace Cubed