#pragma once
#include <glad/glad.h>
#include <Cubed/gameplay/block.hpp>
#include <Cubed/tools/shader_tools.hpp>


class TextureManager {
private:
    GLuint m_block_status_array;
    GLuint m_texture_array; 
    GLuint m_ui_array;
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
};