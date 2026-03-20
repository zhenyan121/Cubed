#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Camera;
class World;
class Renderer {
public:
    constexpr static int NUM_VAO = 1;

    Renderer(const Camera& camera, World& world);
    ~Renderer();
    void init();
    void render(GLuint texture_array);
    void update_proj_matrix(float aspect);
private:
    
    const Camera& m_camera;
    World& m_world;

    glm::mat4 m_p_mat, m_v_mat, m_m_mat, m_mv_mat, m_mvp_mat;
    
    GLuint m_mv_loc;
    GLuint m_proj_loc;

    GLuint m_sky_vbo;
    GLuint m_outline_indices_vbo;
    GLuint m_outline_vbo;

    GLuint m_sky_program;
    GLuint m_outline_program;
    GLuint m_world_program;
    
    std::vector<GLuint> m_vao;
    
    void render_outline();
    void render_sky();
};