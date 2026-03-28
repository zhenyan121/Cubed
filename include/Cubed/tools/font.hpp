#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

struct Character {
    GLuint texture_id;
    glm::ivec2 size;
    glm::ivec2 bearing;
    GLuint advance;
};

class Font {
public:
    Font();
    ~Font();

    static void render_text(GLuint program, GLuint vbo, const std::string& text, float x, float y, float scale, const glm::vec3& color);

private:
    FT_Library m_ft;
    FT_Face m_face;

    std::unordered_map<char8_t, Character> m_characters;

    void load_character(char8_t c);
    void setup_font_character();


};