#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H

#include "Cubed/primitive_data.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

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

    static std::vector<Vertex2D> vertices(const std::string& text,
                                          float x = 0.0f, float y = 0.0f,
                                          float scale = 1.0f);
    static GLuint text_texture();
    static const std::string& font_path();

private:
    FT_Library m_ft;
    FT_Face m_face;

    float m_texture_width = 64;
    float m_texture_height = 64;

    static inline GLuint m_text_texture;
    static inline std::string m_font_path{ASSETS_PATH
                                          "fonts/IBMPlexSans-Regular.ttf"};
    std::unordered_map<char8_t, Character> m_characters;

    void load_character(char8_t c);
    void setup_font_character();
};

} // namespace Cubed