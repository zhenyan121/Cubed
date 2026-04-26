#pragma once
#include <glad/glad.h>
#include <Cubed/gameplay/block.hpp>
#include <Cubed/tools/shader_tools.hpp>

namespace Cubed {


class TextureManager {
private:
    bool m_need_reload = false;
    GLuint m_block_status_array;
    GLuint m_texture_array; 
    GLuint m_ui_array;
    GLfloat m_max_aniso = 0.0f;
    int m_aniso = 1;
    void load_block_status(unsigned status_id);
    void load_block_texture(unsigned block_id);
    void load_ui_texture(unsigned id);

public:
    TextureManager();
    ~TextureManager();
    
    void delet_texture(); 
    GLuint get_block_status_array() const;
    GLuint get_texture_array() const;
    GLuint get_ui_array() const;
    // Must call after MapTable::init_map() and glfwMakeContextCurrent(window);
    void init_texture();
    void hot_reload();
    void need_reload();
    void update();
    int max_aniso() const;
};


}