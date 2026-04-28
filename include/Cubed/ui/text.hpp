#pragma once

#include "Cubed/primitive_data.hpp"
#include "Cubed/ui/color.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
namespace Cubed {

class Shader;

class Text {
public:
    explicit Text(std::string_view name);
    Text(std::string_view name, std::string_view str,
         glm::vec2 pos = glm::vec2{0.0f, 0.0f}, Color color = Color::BLACK);
    ~Text();
    Text(const Text&) = delete;
    Text(Text&&) noexcept;
    Text& operator=(const Text&) = delete;
    Text& operator=(Text&&) noexcept = delete;
    Text& color(Color color);
    // Text& color(const glm::vec4& color, int pos);
    Text& position(float x, float y);
    Text& scale(float s);
    Text& text(std::string_view str);

    std::size_t uuid() const;
    static void set_loc(const Shader& shader);
    void render();

    bool operator==(const Text& other) const;

private:
    float m_scale = 1.0f;
    glm::vec2 m_pos{0.0f, 0.0f};

    const std::string NAME;
    const std::size_t UUID;
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

} // namespace Cubed