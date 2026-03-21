#include <Cubed/camera.hpp>
#include <Cubed/config.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/texture_manager.hpp>
#include <Cubed/renderer.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/shader_tools.hpp>

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
Renderer::Renderer(const Camera& camera, World& world, const TextureManager& texture_manager):
    m_camera(camera),
    m_texture_manager(texture_manager),
    m_world(world)
{
    
}

Renderer::~Renderer() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_outline_vbo);
    glDeleteBuffers(1, &m_outline_indices_vbo);
    glDeleteBuffers(1, &m_sky_vbo);
    glDeleteBuffers(1, &m_ui_vbo);
    glBindVertexArray(0);
    glDeleteVertexArrays(NUM_VAO, m_vao.data());
    glDeleteProgram(m_world_program);
    glDeleteProgram(m_outline_program);
    glDeleteProgram(m_sky_program);
    glDeleteProgram(m_ui_program);
}



void Renderer::init() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG::error("Failed to initialize glad");
        exit(EXIT_FAILURE);
    }
    LOG::info("OpenGL Version: {}.{}", GLVersion.major, GLVersion.minor);
    LOG::info("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    m_world_program = Shader::create_shader_program("shaders/block_v_shader.glsl", "shaders/block_f_shader.glsl");
    m_outline_program = Shader::create_shader_program("shaders/outline_v_shader.glsl", "shaders/outline_f_shader.glsl");
    m_sky_program = Shader::create_shader_program("shaders/sky_v_shader.glsl", "shaders/sky_f_shader.glsl");
    m_ui_program = Shader::create_shader_program("shaders/ui_v_shader.glsl", "shaders/ui_f_shader.glsl");
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    #ifndef NDEBUG    
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param) {
        LOG::info("GL Debug: {}", reinterpret_cast<const char*>(message));
    }, nullptr);
    #endif


    m_vao.resize(NUM_VAO);
    glGenVertexArrays(NUM_VAO, m_vao.data());
    glBindVertexArray(m_vao[0]);
    glBindVertexArray(0);
    glGenBuffers(1, &m_outline_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_outline_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VER), CUBE_VER, GL_STATIC_DRAW);

    glGenBuffers(1, &m_outline_indices_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_outline_indices_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(OUTLINE_CUBE_INDICES), OUTLINE_CUBE_INDICES, GL_STATIC_DRAW);
    
    glGenBuffers(1, &m_sky_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_sky_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES_POS), VERTICES_POS, GL_STATIC_DRAW);

    glGenBuffers(1, &m_ui_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);

    for (int i = 0; i < 6; i++) {
        Vertex2D vex {
            SQUARE_VERTICES[i][0],
            SQUARE_VERTICES[i][1],
            SQUARE_TEXTURE_POS[i][0],
            SQUARE_TEXTURE_POS[i][1],
            0
        };
        m_ui.emplace_back(vex);
    }

    glBufferData(GL_ARRAY_BUFFER, m_ui.size() * sizeof(Vertex2D), m_ui.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::render() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(m_vao[0]);
    render_sky();
    glUseProgram(m_world_program);
    
    m_mv_loc = glGetUniformLocation(m_world_program, "mv_matrix");
    m_proj_loc = glGetUniformLocation(m_world_program, "proj_matrix");
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_manager.get_texture_array());
    m_m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    m_v_mat = m_camera.get_camera_lookat();
    m_mv_mat = m_v_mat * m_m_mat;  
    glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_mv_mat));
    glUniformMatrix4fv(m_proj_loc, 1 ,GL_FALSE, glm::value_ptr(m_p_mat));
    m_mvp_mat = m_p_mat * m_mv_mat;
    m_world.render(m_mvp_mat);
    
    render_outline();

    render_ui();
}

void Renderer::render_outline() {
    glUseProgram(m_outline_program);

    m_mv_loc = glGetUniformLocation(m_outline_program, "mv_matrix");
    m_proj_loc = glGetUniformLocation(m_outline_program, "proj_matrix");
    
    const auto& block_pos = m_world.get_look_block_pos("TestPlayer");

    if (block_pos != std::nullopt) {
        m_m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(block_pos.value().pos));
        m_mv_mat = m_v_mat * m_m_mat;
        glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_mv_mat));
        glUniformMatrix4fv(m_proj_loc, 1 ,GL_FALSE, glm::value_ptr(m_p_mat));

        glBindBuffer(GL_ARRAY_BUFFER, m_outline_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_outline_indices_vbo);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glLineWidth(4.0f);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }
    
}

void Renderer::render_sky() {
    glUseProgram(m_sky_program);

    m_mv_loc = glGetUniformLocation(m_sky_program, "mv_matrix");
    m_proj_loc = glGetUniformLocation(m_sky_program, "proj_matrix");

    m_m_mat = glm::translate(glm::mat4(1.0f), m_camera.get_camera_pos() - glm::vec3(0.5f, 0.5f, 0.5f));
    m_v_mat = m_camera.get_camera_lookat();
    m_mv_mat = m_v_mat * m_m_mat;

    glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_mv_mat));
    glUniformMatrix4fv(m_proj_loc, 1 ,GL_FALSE, glm::value_ptr(m_p_mat));

    glBindBuffer(GL_ARRAY_BUFFER, m_sky_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glEnable(GL_DEPTH_TEST);
    
}

void Renderer::render_ui() {
    glUseProgram(m_ui_program);
    glDisable(GL_DEPTH_TEST);

    m_mv_loc = glGetUniformLocation(m_ui_program, "m_matrix");
    m_proj_loc = glGetUniformLocation(m_ui_program, "proj_matrix");

    glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_ui_m_matrix));
    glUniformMatrix4fv(m_proj_loc, 1, GL_FALSE, glm::value_ptr(m_ui_proj));

    glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, s));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, layer));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_manager.get_ui_array());

    glDrawArrays(GL_TRIANGLES, 0, 6);
    Shader::check_opengl_error();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnable(GL_DEPTH_TEST);

}

void Renderer::update_proj_matrix(float aspect, float width, float height) {
    m_p_mat = glm::perspective(glm::radians(FOV), aspect, 0.1f, 1000.0f); 
    m_ui_proj = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    // scale and then translate
    m_ui_m_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(width / 2.0f, height / 2.0f, 0.0)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 50.0f, 1.0f));
    
}