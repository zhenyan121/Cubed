#include <Cubed/config.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/texture_manager.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
TextureManager::TextureManager() {

}

TextureManager::~TextureManager() {
    delet_texture();   
}

const BlockTexture& TextureManager::get_block_texture(const std::string& name) {


    load_block_texture(name);
    return m_block_textures[MapTable::get_id_from_name(name)];
    
}

const BlockTexture& TextureManager::get_block_texture(unsigned id) {


    load_block_texture(id);
    return m_block_textures[id];
    
}


void TextureManager::delet_texture() {
    for (const auto& block_texture : m_block_textures) {  
        for (const auto& id : block_texture.texture) {
            glDeleteTextures(1, &id);
        }
    }
    LOG::info("Successfully delete all texture");
}

void TextureManager::load_block_texture(const std::string& block_name) {

    auto id = MapTable::get_id_from_name(block_name);
    m_block_textures[id].name = block_name;
    m_block_textures[id].id = id;
    // air don`t need texture
    if (id == 0) {
        return;
    }
    std::string block_texture_path = "assets/texture/block/" + block_name;
    m_block_textures[id].texture.emplace_back(Shader::load_texture(block_texture_path + "/front.png"));
    m_block_textures[id].texture.emplace_back(Shader::load_texture(block_texture_path + "/right.png"));
    m_block_textures[id].texture.emplace_back(Shader::load_texture(block_texture_path + "/back.png"));
    m_block_textures[id].texture.emplace_back(Shader::load_texture(block_texture_path + "/left.png"));
    m_block_textures[id].texture.emplace_back(Shader::load_texture(block_texture_path + "/top.png"));
    m_block_textures[id].texture.emplace_back(Shader::load_texture(block_texture_path + "/base.png"));
}

void TextureManager::load_block_texture(unsigned block_id) {
    CUBED_ASSERT_MSG(block_id < MAX_BLOCK_NUM, "Exceed the max block sum limit");
    load_block_texture(MapTable::get_name_from_id(block_id));
}

void TextureManager::init_texture() {
    MapTable::init_map();
    m_block_textures.resize(MAX_BLOCK_NUM);

    for (int i = 0; i < MAX_BLOCK_NUM; i++) {
        load_block_texture(i);
    }

}