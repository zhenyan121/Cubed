
#include <Cubed/shader.hpp>
#include <Cubed/tools/font.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/shader_tools.hpp>


Font::Font() {

    if (FT_Init_FreeType(&m_ft)) {
        Logger::error("FREETYPE: Could not init FreeType Library");
    }
    if (FT_New_Face(m_ft, "assets/fonts/IBMPlexSans-Regular.ttf", 0, &m_face)) {
        Logger::error("FREETYPE: Failed to load font");
    }

    FT_Set_Pixel_Sizes(m_face, 0, 48); 
    setup_font_character();
}

Font::~Font() {
    
    FT_Done_Face(m_face);
    FT_Done_FreeType(m_ft);
    glDeleteTextures(1, &m_text_texture);
}

void Font::load_character(char8_t c) {
    if (FT_Load_Char(m_face, c, FT_LOAD_RENDER)) {
            Logger::error("FREETYTPE: Failed to load Glyph");
            return;
        }
        const auto& width = m_face->glyph->bitmap.width;
        const auto& height = m_face->glyph->bitmap.rows;
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_text_texture);
        glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY,
            0,
            0,
            0,
            static_cast<int>(c),
            width,
            height,
            1,
            GL_RED,
            GL_UNSIGNED_BYTE,
            m_face->glyph->bitmap.buffer
        );

        Character character = {
            glm::vec2{0.0f, 0.0f},
            glm::vec2{static_cast<float>(width) / m_texture_width, static_cast<float>(height) / m_texture_height},
            glm::ivec2(m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows),
            glm::ivec2(m_face->glyph->bitmap_left, m_face->glyph->bitmap_top),
            static_cast<GLuint>(m_face->glyph->advance.x)
        };

        m_characters.insert({c, std::move(character)});
}

void Font::setup_font_character() {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    glGenTextures(1, &m_text_texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_text_texture);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_RED,
        m_texture_width,
        m_texture_height,
        MAX_CHARACTER,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    for (char8_t c = 0; c < 128; c++) {
        load_character(c);
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0); 
}

std::vector<Vertex2D> Font::vertices(const std::string &text, float x, float y, float scale) {
    static Font font;
    
    std::vector<Vertex2D> vertices;

    for (char8_t c : text) {
        auto it = font.m_characters.find(c);
        if (it == font.m_characters.end()) {
            Logger::error("Can't find character {}", static_cast<char>(c));
            continue;
        }
        Character& ch = it->second;
        float xpos = x + ch.bearing.x * scale;
        float ypos = y - ch.bearing.y * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        vertices.emplace_back(xpos,         ypos + h, ch.uv_min.x, ch.uv_max.y, static_cast<float>(c)); 
        vertices.emplace_back(xpos,         ypos,     ch.uv_min.x, ch.uv_min.y, static_cast<float>(c)); 
        vertices.emplace_back(xpos + w,     ypos,     ch.uv_max.x, ch.uv_min.y, static_cast<float>(c)); 
        vertices.emplace_back(xpos,         ypos + h, ch.uv_min.x, ch.uv_max.y, static_cast<float>(c)); 
        vertices.emplace_back(xpos + w,     ypos,     ch.uv_max.x, ch.uv_min.y, static_cast<float>(c)); 
        vertices.emplace_back(xpos + w,     ypos + h, ch.uv_max.x, ch.uv_max.y, static_cast<float>(c));
        
        x += (ch.advance >> 6) * scale;
        
    }

    return vertices;
}

GLuint Font::text_texture() {
    return m_text_texture;
}