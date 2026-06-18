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
    constexpr static int NUM_VAO = 7;

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
    float& ambient_strength();

    bool& discard_transparent();
    bool& shader_on();
    int& shadow_mode();
    int& light_cull_face();
    int& light_size_uv();
    float& min_radius();
    float& max_radius();
    int& samples();
    float& specular_strength();

private:
    static constexpr glm::vec3 SUNLIGHT_COLOR{1.0f, 1.0f, 1.0f};
    static constexpr glm::vec3 SUN_COLOR{1.00f, 0.95f, 0.80f};
    static constexpr glm::vec3 MOON_COLOR{0.75f, 0.80f, 1.00f};
    static constexpr glm::vec3 SKY_COLOR{0.529, 0.808, 0.922};
    static constexpr float FAR_PLANE = 1000.0f;
    static constexpr float NEAR_PLANE = 0.1f;
    static constexpr float SUN_SIZE = 50.0f;
    static constexpr float MOON_SIZE = 50.0f;
    static constexpr float DEPTH_MAP_SIZE = 4096.0f;
    static constexpr float ANGLE_STEP_DEG = 0.5f;
    float m_ambient_strength = 0.1f;

    const Camera& m_camera;
    DevPanel& m_dev_panel;
    const TextureManager& m_texture_manager;
    World& m_world;

    bool m_discard_tranparent = true;
    bool m_shader_on = true;
    int m_shadow_mode = 0;
    int m_light_cull_face = 0;
    float m_aspect = 0.0f;
    float m_fov = DEFAULT_FOV;

    float m_delta_time = 0.0f;

    float m_width = 0.0f;
    float m_height = 0.0f;

    glm::mat4 m_p_mat, m_v_mat, m_m_mat, m_mv_mat, m_mvp_mat, m_norm_mat;

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

    GLuint m_oit_fbo = 0;
    GLuint m_accum_texture = 0;
    GLuint m_reveal_texture = 0;
    GLuint m_oit_depth_render_buffer = 0;

    GLuint m_depth_map_fbo = 0;
    GLuint m_depth_map_texture = 0;

    GLuint m_quad_vbo = 0;

    glm::mat4 m_ui_proj;
    glm::mat4 m_ui_m_matrix;
    std::unordered_map<std::size_t, Shader> m_shaders;

    glm::vec3 m_blend_from_sundir;
    glm::vec3 m_blend_to_sundir;
    float m_blend_t = 1.0f;
    bool m_blend_initialized = false;
    static constexpr float BLEND_DURATION = 0.15f;
    int m_light_size_uv = 20;

    float m_min_radius = 2.0f;
    float m_max_radius = 20.0f;
    int m_samples = 16;

    float m_specular_strength = 0.5f;

    /*
    0 - quad vao
    1 - sky vao
    2 - outline vao
    3 - ui vao
    4 - text vao

    */
    std::vector<GLuint> m_vao;
    std::vector<Vertex2D> m_ui;

    void init_quad();
    void init_text();

    void render_outline();
    void render_sky();
    void render_text();
    void render_ui();
    void render_world();
    void render_underwater();
    void render_dev_panel();

    glm::vec3 quantize_sun_direction(const glm::vec3& sundir,
                                     float angle_step_deg) const;
    glm::vec3 get_smoothed_shadow_sundir(const glm::vec3& raw_shadow_sundir,
                                         float dt);
};

} // namespace Cubed