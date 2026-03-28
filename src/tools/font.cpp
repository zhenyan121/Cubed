

#include <Cubed/tools/font.hpp>
#include <Cubed/tools/log.hpp>




Font::Font() {

    if (FT_Init_FreeType(&m_ft)) {
        LOG::error("FREETYPE: Could not init FreeType Library");
    }
    if (FT_New_Face(m_ft, "assets/fonts/IBMPlexSans-Regular.ttf", 0, &m_face)) {
        LOG::error("FREETYPE: Failed to load font");
    }

    FT_Set_Pixel_Sizes(m_face, 0, 48); 
    setup_font_character();
}

Font::~Font() {
    
    FT_Done_Face(m_face);
    FT_Done_FreeType(m_ft);
    for (const auto& [key, character] : m_characters) {
        glDeleteTextures(1, &character.texture_id);
    }
}

void Font::load_character(char8_t c) {
    if (FT_Load_Char(m_face, c, FT_LOAD_RENDER)) {
            LOG::error("FREETYTPE: Failed to load Glyph");
            return;
        }
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            m_face->glyph->bitmap.width,
            m_face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            m_face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows),
            glm::ivec2(m_face->glyph->bitmap_left, m_face->glyph->bitmap_top),
            static_cast<GLuint>(m_face->glyph->advance.x)
        };

        m_characters.insert({c, std::move(character)});
}

void Font::setup_font_character() {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  
    for (char8_t c = 0; c < 128; c++) {
        load_character(c);
    }
}



void Font::render_text(GLuint program, GLuint vbo, const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    static Font font;
    glUseProgram(program);
    glUniform3f(glGetUniformLocation(program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    for (char8_t c : text) {
        auto it = font.m_characters.find(c);
        if (it == font.m_characters.end()) {
            LOG::error("Can't find character {}", static_cast<char>(c));
            continue;
        }
        Character& ch = it->second;
        float xpos = x + ch.bearing.x * scale;
        float ypos = y - ch.bearing.y * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        float vertices[6][4] {
        { xpos,     ypos + h,   0.0, 1.0 },  
        { xpos,     ypos,       0.0, 0.0 },  
        { xpos + w, ypos,       1.0, 0.0 },  

        { xpos,     ypos + h,   0.0, 1.0 },  
        { xpos + w, ypos,       1.0, 0.0 },  
        { xpos + w, ypos + h,   1.0, 1.0 }  
        };
        glBindTexture(GL_TEXTURE_2D, ch.texture_id);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.advance >> 6) * scale;
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

}
