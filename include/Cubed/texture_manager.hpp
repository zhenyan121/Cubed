#pragma once
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <Cubed/tools/shader_tools.hpp>
struct BlockTexture {
    std::string name;
    std::vector<GLuint> texture;

};

class TextureManager {
private:
    static std::size_t make_hash(std::string);
    std::unordered_map<std::size_t, BlockTexture> m_block_textures;

public:
    TextureManager();
    ~TextureManager();
    const BlockTexture& get_block_texture(std::string name);
    void delet_texture();
    
    void load_block_texture(std::string block_name);
};