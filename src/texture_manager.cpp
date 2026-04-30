#include "Cubed/texture_manager.hpp"

#include "Cubed/config.hpp"
#include "Cubed/constants.hpp"
#include "Cubed/map_table.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/shader_tools.hpp"

namespace Cubed {

TextureManager::TextureManager() {}

TextureManager::~TextureManager() { delet_texture(); }

void TextureManager::delet_texture() {
    glDeleteTextures(1, &m_texture_array);
    glDeleteTextures(1, &m_block_status_array);
    for (auto& id : m_item_textures) {
        glDeleteTextures(1, &id);
    }
    Logger::info("Successfully delete all texture");
}

GLuint TextureManager::get_block_status_array() const {
    return m_block_status_array;
}

GLuint TextureManager::get_texture_array() const { return m_texture_array; }

GLuint TextureManager::get_ui_array() const { return m_ui_array; }

const std::vector<GLuint>& TextureManager::item_textures() const {
    return m_item_textures;
}

void TextureManager::load_block_status(unsigned id) {

    ASSERT_MSG(id < MAX_BLOCK_STATUS, "Exceed the max status sum limit");
    std::string path = "texture/status/" + std::to_string(id) + ".png";
    unsigned char* image_data = nullptr;
    image_data = (Tools::load_image_data(path));
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_block_status_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, id, 16, 16, 1, GL_RGBA,
                    GL_UNSIGNED_BYTE, image_data);
    Tools::delete_image_data(image_data);
}

void TextureManager::load_block_texture(unsigned id) {
    ASSERT_MSG(id < MAX_BLOCK_NUM, "Exceed the max block sum limit");
    std::string name{MapTable::get_name_from_id(id)};
    // air don`t need texture
    if (id == 0) {
        return;
    }
    unsigned char* image_data[6];

    std::string block_texture_path = "texture/block/" + name;
    image_data[0] = (Tools::load_image_data(block_texture_path + "/front.png"));
    image_data[1] = (Tools::load_image_data(block_texture_path + "/right.png"));
    image_data[2] = (Tools::load_image_data(block_texture_path + "/back.png"));
    image_data[3] = (Tools::load_image_data(block_texture_path + "/left.png"));
    image_data[4] = (Tools::load_image_data(block_texture_path + "/top.png"));
    image_data[5] = (Tools::load_image_data(block_texture_path + "/base.png"));

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_array);
    Tools::check_opengl_error();
    for (int i = 0; i < 6; i++) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, id * 6 + i, 16, 16, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE, image_data[i]);
        Tools::check_opengl_error();
        Tools::delete_image_data(image_data[i]);
    }
}

void TextureManager::load_item_texture(const std::string& name) {
    std::string path = "texture/item/block/" + name + ".png";
    unsigned char* data = nullptr;
    data = Tools::load_image_data(path);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16, 16, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    if (m_aniso >= 1) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY,
                        static_cast<GLfloat>(m_aniso));
    }
    m_item_textures.push_back(texture);
    Tools::delete_image_data(data);
}

void TextureManager::load_ui_texture(unsigned id) {
    ASSERT_MSG(id < MAX_UI_NUM, "Exceed the max ui sum limit");

    std::string path = "texture/ui/" + std::to_string(id) + ".png";
    unsigned char* image_data = nullptr;
    image_data = (Tools::load_image_data(path));
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ui_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, id, 16, 16, 1, GL_RGBA,
                    GL_UNSIGNED_BYTE, image_data);
    Tools::delete_image_data(image_data);
}

void TextureManager::init_item() {
    auto& map = MapTable::item_map();
    for (const auto& name : map) {
        load_item_texture(name);
    }
}
void TextureManager::init_block() {
    glGenTextures(1, &m_texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_array);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 16, 16, MAX_BLOCK_NUM * 6, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    for (int i = 0; i < MAX_BLOCK_NUM; i++) {
        load_block_texture(i);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_array);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    if (m_aniso >= 1) {
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY,
                        static_cast<GLfloat>(m_aniso));
    }
    Logger::info("Block Texture Load Success");
}
void TextureManager::init_ui() {
    glGenTextures(1, &m_ui_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ui_array);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 16, 16, MAX_UI_NUM, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    for (int i = 0; i < MAX_UI_NUM; i++) {
        load_ui_texture(i);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ui_array);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}
void TextureManager::init_block_status() {
    glGenTextures(1, &m_block_status_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_block_status_array);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 16, 16, MAX_BLOCK_STATUS, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    for (int i = 0; i < MAX_BLOCK_STATUS; i++) {
        load_block_status(i);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_block_status_array);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    if (m_aniso >= 1) {
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY,
                        static_cast<GLfloat>(m_aniso));
    }
}
void TextureManager::init_texture() {
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &m_max_aniso);
    if (m_max_aniso > 0.0f) {
        Logger::info("Support anisotropic filtering max_aniso is {}",
                     m_max_aniso);
    }
    m_aniso = Config::get().get<int>("texture.aniso");
    m_aniso = std::min(static_cast<int>(m_max_aniso), m_aniso);
    Logger::info("Setting Texture Aniso is {}", m_aniso);
    MapTable::init_map();
    Logger::info("Map Init Success");

    init_block();
    init_block_status();
    init_ui();
    init_item();
}

void TextureManager::update() {
    if (m_need_reload) {
        hot_reload();
    }
}

void TextureManager::need_reload() { m_need_reload = true; }

void TextureManager::hot_reload() {
    delet_texture();

    init_texture();
    m_need_reload = false;
}

int TextureManager::max_aniso() const { return static_cast<int>(m_max_aniso); }

} // namespace Cubed