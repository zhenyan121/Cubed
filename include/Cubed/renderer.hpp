#pragma once

#include "Cubed/constants.hpp"
#include "Cubed/primitive_data.hpp"
#include "Cubed/shader.hpp"
#include "Cubed/ui/text.hpp"

#include <glm/glm.hpp>
#include <vector>
namespace Cubed {

class Camera;
class TextureManager;
class World;
class DevPanel;
class Renderer {
public:
    constexpr static int NUM_VAO = 6;

    Renderer(const Camera& camera, World& world,
             const TextureManager& texture_manager, DevPanel& dev_panel);
    ~Renderer();
    void hot_reload();
    void init();
    const Shader& get_shader(const std::string& name) const;
    void render();
    void update(float delta_time);
    void update_fov(float fov);
    void update_proj_matrix(float aspect, float width, float height);
    void updata_framebuffer(int width, int height);

private:
    const Camera& m_camera;
    DevPanel& m_dev_panel;
    const TextureManager& m_texture_manager;
    World& m_world;

    float m_aspect = 0.0f;
    float m_fov = DEFAULT_FOV;

    float m_delta_time = 0.0f;

    glm::mat4 m_p_mat, m_v_mat, m_m_mat, m_mv_mat, m_mvp_mat;

    GLuint m_mv_loc = 0;
    GLuint m_proj_loc = 0;

    GLuint m_sky_vbo = 0;
    GLuint m_text_vbo = 0;
    GLuint m_outline_indices_vbo = 0;
    GLuint m_outline_vbo = 0;
    GLuint m_ui_vbo = 0;

    GLuint m_fbo = 0;
    GLuint m_screen_texture = 0;
    GLuint m_depth_render_buffer = 0;

    GLuint m_quad_vbo = 0;

    glm::mat4 m_ui_proj;
    glm::mat4 m_ui_m_matrix;
    std::unordered_map<std::size_t, Shader> m_shaders;
    std::vector<GLuint> m_vao;
    std::vector<Vertex2D> m_ui;

    void init_underwater();
    void init_text();

    void render_outline();
    void render_sky();
    void render_text();
    void render_ui();
    void render_world();
    void render_underwater();
    void render_dev_panel();
};

} // namespace Cubed