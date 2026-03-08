#pragma once
#include <glad/glad.h>
#include <Cubed/gameplay/block.hpp>
#include <Cubed/tools/shader_tools.hpp>


class TextureManager {
private:
    GLuint m_texture_array;

    void load_block_texture(unsigned block_id);

public:
    TextureManager();
    ~TextureManager();

    void delet_texture(); 
    GLuint get_texture_array();
    // Must call after MapTable::init_map() and glfwMakeContextCurrent(window);
    void init_texture();
};