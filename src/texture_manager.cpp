#include <Cubed/texture_manager.hpp>
#include <Cubed/constants.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>

namespace Cubed {


TextureManager::TextureManager() {

}

TextureManager::~TextureManager() {
    delet_texture();   
}

void TextureManager::delet_texture() {
    glDeleteTextures(1, &m_texture_array);
    glDeleteTextures(1, &m_block_status_array);
    Logger::info("Successfully delete all texture");
}

GLuint TextureManager::get_block_status_array() const{
    return m_block_status_array;
}

GLuint TextureManager::get_texture_array() const{
    return m_texture_array;
}

GLuint TextureManager::get_ui_array() const{
    return m_ui_array;
}

void TextureManager::load_block_status(unsigned id) {

    ASSERT_MSG(id < MAX_BLOCK_STATUS, "Exceed the max status sum limit");
    std::string path = "texture/status/" + std::to_string(id) + ".png";
    unsigned char* image_data = nullptr;
    image_data = (Tools::load_image_data(path));
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_block_status_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0 ,0, id,
                        16, 16, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        image_data
                    );
    Tools::delete_image_data(image_data);
}

void TextureManager::load_block_texture(unsigned id) {
    ASSERT_MSG(id < MAX_BLOCK_NUM, "Exceed the max block sum limit");
    const std::string& name = MapTable::get_name_from_id(id);
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
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
            0, 0, id * 6 + i,
            16, 16, 1,
            GL_RGBA, GL_UNSIGNED_BYTE,
            image_data[i]
            );
        Tools::check_opengl_error();
        Tools::delete_image_data(image_data[i]);
    }

}

void TextureManager::load_ui_texture(unsigned id) {
    ASSERT_MSG(id < MAX_UI_NUM, "Exceed the max ui sum limit");

    std::string path = "texture/ui/" + std::to_string(id) + ".png";
    unsigned char* image_data = nullptr;
    image_data = (Tools::load_image_data(path));
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ui_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0 ,0, id,
                        16, 16, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        image_data
                    );
    Tools::delete_image_data(image_data);

}

void TextureManager::init_texture() {

    MapTable::init_map();
    Logger::info("Map Init Success");
    glGenTextures(1, &m_texture_array);
    Tools::check_opengl_error();
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_array);
    Tools::check_opengl_error();
    glTexImage3D(GL_TEXTURE_2D_ARRAY,
        0, GL_RGBA,
        16, 16,
        MAX_BLOCK_NUM * 6,
        0, GL_RGBA,
        GL_UNSIGNED_BYTE, nullptr);
    Tools::check_opengl_error();
    for (int i = 0; i < MAX_BLOCK_NUM; i++) {
        load_block_texture(i);
    }
    Logger::info("Block Texture Load Success");
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_array);
    Tools::check_opengl_error();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    Tools::check_opengl_error();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    Tools::check_opengl_error();
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    Tools::check_opengl_error();

    GLfloat max_aniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
    if (max_aniso > 0.0f) {
        Logger::info("Support anisotropic filtering max_aniso is {}", max_aniso);
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
    }    

    glGenTextures(1, &m_block_status_array);
    Tools::check_opengl_error();
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_block_status_array);
    Tools::check_opengl_error();
    glTexImage3D(GL_TEXTURE_2D_ARRAY,
        0, GL_RGBA,
        16, 16,
        MAX_BLOCK_STATUS,
        0, GL_RGBA,
        GL_UNSIGNED_BYTE, nullptr);
    Tools::check_opengl_error();
    for (int i = 0; i < MAX_BLOCK_STATUS; i++) {
        load_block_status(i);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_block_status_array);
    Tools::check_opengl_error();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    Tools::check_opengl_error();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    Tools::check_opengl_error();
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    Tools::check_opengl_error();

    if (max_aniso > 0.0f) {
        Logger::info("Support anisotropic filtering max_aniso is {}", max_aniso);
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
    }
    
    glGenTextures(1, &m_ui_array);
    Tools::check_opengl_error();
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ui_array);
    Tools::check_opengl_error();
    glTexImage3D(GL_TEXTURE_2D_ARRAY,
        0, GL_RGBA,
        16, 16,
        MAX_UI_NUM,
        0, GL_RGBA,
        GL_UNSIGNED_BYTE, nullptr);
    Tools::check_opengl_error();
    for (int i = 0; i < MAX_UI_NUM; i++) {
        load_ui_texture(i);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_ui_array);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 


}

void TextureManager::update() {
    if (m_need_reload) {
        hot_reload();
    }
}

void TextureManager::need_reload() {
    m_need_reload = true;
}

void TextureManager::hot_reload() {
    delet_texture();
    init_texture();
    m_need_reload = false;
}

}