#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

#include <Cubed/config.hpp>
#include <Cubed/primitive_data.hpp>

namespace Cubed {


struct Character {
    glm::vec2 uv_min;
    glm::vec2 uv_max;
    glm::ivec2 size;
    glm::ivec2 bearing;
    GLuint advance;
};

class Shader;

class Font {
public:
    Font();
    ~Font();

    static std::vector<Vertex2D> vertices(const std::string& text, float x = 0.0f, float y = 0.0f, float scale = 1.0f);
    static GLuint text_texture();
    static const std::filesystem::path& font_path();
private:
    FT_Library m_ft;
    FT_Face m_face;

    float m_texture_width = 64;
    float m_texture_height = 64;

    static inline GLuint m_text_texture;
    static inline std::filesystem::path m_font_path{ASSETS_PATH "fonts/IBMPlexSans-Regular.ttf"};
    std::unordered_map<char8_t, Character> m_characters;

    void load_character(char8_t c);
    void setup_font_character();


};

}