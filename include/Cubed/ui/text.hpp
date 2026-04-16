#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <Cubed/config.hpp>
#include <Cubed/ui/color.hpp>

class Shader;

class Text {
public:
    Text();
    Text(std::string_view str, glm::vec2 pos = glm::vec2{0.0f, 0.0f}, Color color = Color::BLACK);
    ~Text();

    Text& color(Color color);
    //Text& color(const glm::vec4& color, int pos);
    Text& position(float x, float y);
    Text& scale(float s);
    static void set_loc(const Shader& shader);
    Text& text(std::string_view str);
    

    void render();

private:
    float m_scale = 1.0f;
    glm::vec2 m_pos{0.0f, 0.0f};

    std::string m_name;
    std::string m_text;
    glm::vec4 m_color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::mat4 m_model_matrix;
    
    std::vector<Vertex2D> m_vertices;
    GLuint m_vbo = 0;
    static inline GLuint m_color_loc = 0;
    static inline GLuint m_mv_loc = 0;

    void update_vertices();
    void upload_to_gpu();

};