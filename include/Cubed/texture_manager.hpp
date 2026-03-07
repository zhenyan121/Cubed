#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include <Cubed/gameplay/block.hpp>
#include <Cubed/tools/shader_tools.hpp>


class TextureManager {
private:
    std::vector<BlockTexture> m_block_textures;

public:
    TextureManager();
    ~TextureManager();
    const BlockTexture& get_block_texture(const std::string& block_name);
    const BlockTexture& get_block_texture(unsigned block_id);
    void delet_texture();
    
    void load_block_texture(const std::string& block_name);
    void load_block_texture(unsigned block_id);
    // Must call after MapTable::init_map() and glfwMakeContextCurrent(window);
    void init_texture();
};