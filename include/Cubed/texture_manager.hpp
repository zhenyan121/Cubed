#pragma once
#include "Cubed/gameplay/block.hpp"

#include <glad/glad.h>

namespace Cubed {

class TextureManager {
private:
    bool m_need_reload = false;
    GLuint m_block_status_array;
    GLuint m_texture_array;
    GLuint m_ui_array;
    GLfloat m_max_aniso = 0.0f;
    int m_aniso = 1;

    std::vector<GLuint> m_item_textures;
    void load_block_status(unsigned status_id);
    void load_block_texture(unsigned block_id);
    void load_item_texture(const std::string& name);
    void load_ui_texture(unsigned id);
    void init_item();
    void init_block();
    void init_ui();
    void init_block_status();

public:
    TextureManager();
    ~TextureManager();

    void delet_texture();
    GLuint get_block_status_array() const;
    GLuint get_texture_array() const;
    GLuint get_ui_array() const;
    const std::vector<GLuint>& item_textures() const;
    // Must call after MapTable::init_map() and glfwMakeContextCurrent(window);
    void init_texture();
    void hot_reload();
    void need_reload();
    void update();
    int max_aniso() const;
};

} // namespace Cubed