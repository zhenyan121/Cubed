#include <Cubed/texture_manager.hpp>

TextureManager::TextureManager() {

}

TextureManager::~TextureManager() {
    delet_texture();   
}

std::size_t TextureManager::make_hash(std::string name) {
    std::size_t h1 = std::hash<std::string>{}(name);
    return h1; 
}

const BlockTexture& TextureManager::get_block_texture(std::string name) {
    auto it = m_block_textures.find(make_hash(name));
    if (it != m_block_textures.end()) {
        return it->second;
    }
    load_block_texture(name);
    return m_block_textures[make_hash(name)];
    
}


void TextureManager::delet_texture() {
    for (const auto& texture : m_block_textures) {
        auto [key, block_texture] = texture;
        for (const GLuint& texture_id : block_texture.texture) {
            glDeleteTextures(1, &texture_id);
        }        
    }
}

void TextureManager::load_block_texture(std::string block_name) {
    BlockTexture block_texture;
    std::string block_texture_path = "assets/texture/block/" + block_name;
    block_texture.texture.emplace_back(load_texture(block_texture_path + "/front.png"));
    block_texture.texture.emplace_back(load_texture(block_texture_path + "/right.png"));
    block_texture.texture.emplace_back(load_texture(block_texture_path + "/back.png"));
    block_texture.texture.emplace_back(load_texture(block_texture_path + "/left.png"));
    block_texture.texture.emplace_back(load_texture(block_texture_path + "/top.png"));
    block_texture.texture.emplace_back(load_texture(block_texture_path + "/base.png"));
    m_block_textures[make_hash(block_name)] = block_texture;
}