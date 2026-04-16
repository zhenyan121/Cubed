#include <Cubed/ui/text.hpp>

#include <Cubed/shader.hpp>
#include <Cubed/tools/font.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
Text::Text() {
    
}

Text::Text(std::string_view str, glm::vec2 pos, Color color) {
    m_text.assign(str);
    m_pos = pos;
    m_color = color_value(color);
    update_vertices();
}

Text::~Text() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
}

Text& Text::color(Color color) {
    m_color = color_value(color);
    return *this;
}

Text& Text::position(float x, float y) {
    m_pos = glm::vec2{x, y};
    return *this;
}

Text& Text::scale(float s) {
    m_scale = s;
    return *this;
}

void Text::set_loc(const Shader& shader) {
    m_color_loc = shader.loc("textColor");
    m_mv_loc = shader.loc("mv_matrix");
}

Text& Text::text(std::string_view str) {
    m_text.assign(str);
    update_vertices();
    return *this;
}

void Text::render() {

    CUBED_ASSERT_MSG(!m_vertices.empty(), "Text String Not Set");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Font::text_texture());
    CUBED_ASSERT_MSG(m_color_loc, "m_color_loc is null");
    
    m_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(m_pos.x, m_pos.y, 0.0f)) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(m_scale, m_scale, 1.0f));
    
    glUniform3f(m_color_loc, m_color.x, m_color.y, m_color.z);
    glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_model_matrix));
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, s));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, layer));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void Text::update_vertices() {
    m_vertices = Font::vertices(m_text);
    upload_to_gpu();
}

void Text::upload_to_gpu() {
    if (m_vbo == 0) {
        glGenBuffers(1, &m_vbo);
    }
    CUBED_ASSERT_MSG(m_vbo, "Vbo Is Not Gen");
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex2D), m_vertices.data(), GL_DYNAMIC_DRAW);
}