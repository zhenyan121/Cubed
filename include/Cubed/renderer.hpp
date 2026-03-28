#pragma once

#include <Cubed/config.hpp>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Camera;
class TextureManager;
class World;
class Renderer {
public:
    constexpr static int NUM_VAO = 1;

    Renderer(const Camera& camera, World& world, const TextureManager& texture_manager);
    ~Renderer();
    void init();
    void render();
    void update_proj_matrix(float aspect, float width, float height);
private:
    
    const Camera& m_camera;
    const TextureManager& m_texture_manager;
    World& m_world;

    glm::mat4 m_p_mat, m_v_mat, m_m_mat, m_mv_mat, m_mvp_mat;
    
    GLuint m_mv_loc;
    GLuint m_proj_loc;

    GLuint m_sky_vbo;
    GLuint m_text_vbo;
    GLuint m_outline_indices_vbo;
    GLuint m_outline_vbo;
    GLuint m_ui_vbo;

    GLuint m_sky_program;
    GLuint m_text_program;
    GLuint m_outline_program;
    GLuint m_ui_program;
    GLuint m_world_program;
    

    glm::mat4 m_ui_proj;
    glm::mat4 m_ui_m_matrix;

    std::vector<GLuint> m_vao;
    std::vector<Vertex2D> m_ui;
    void render_outline();
    void render_sky();
    void render_text(); 
    void render_ui();
    
};