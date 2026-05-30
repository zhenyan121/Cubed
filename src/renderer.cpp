#include "Cubed/renderer.hpp"

#include "Cubed/camera.hpp"
#include "Cubed/config.hpp"
#include "Cubed/debug_collector.hpp"
#include "Cubed/dev_panel.hpp"
#include "Cubed/gameplay/player.hpp"
#include "Cubed/gameplay/world.hpp"
#include "Cubed/primitive_data.hpp"
#include "Cubed/texture_manager.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/cubed_hash.hpp"
#include "Cubed/tools/font.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/shader_tools.hpp"

#include <GLFW/glfw3.h>
#include <format>
#include <glm/gtc/type_ptr.hpp>
namespace Cubed {

Renderer::Renderer(const Camera& camera, World& world,
                   const TextureManager& texture_manager, DevPanel& dev_panel)
    : m_camera(camera), m_dev_panel(dev_panel),
      m_texture_manager(texture_manager), m_world(world) {}

Renderer::~Renderer() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_outline_vbo);
    glDeleteBuffers(1, &m_outline_indices_vbo);
    glDeleteBuffers(1, &m_sky_vbo);
    glDeleteBuffers(1, &m_ui_vbo);
    glDeleteBuffers(1, &m_text_vbo);
    glBindVertexArray(0);
    glDeleteVertexArrays(NUM_VAO, m_vao.data());
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_screen_texture);
    glDeleteRenderbuffers(1, &m_depth_render_buffer);
}

void Renderer::hot_reload() {
    auto& config = Config::get();
    update_fov(config.get<double>("player.fov"));
}

void Renderer::init() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::error("Failed to initialize glad");
        exit(EXIT_FAILURE);
    }
    Logger::info("OpenGL Version: {}.{}", GLVersion.major, GLVersion.minor);
    Logger::info("Renderer: {}",
                 reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    Shader world_shader{"world", "shaders/block_v_shader.glsl",
                        "shaders/block_f_shader.glsl"};
    Shader outline_shader{"outline", "shaders/outline_v_shader.glsl",
                          "shaders/outline_f_shader.glsl"};
    Shader sky_shdaer{"sky", "shaders/sky_v_shader.glsl",
                      "shaders/sky_f_shader.glsl"};
    Shader ui_shdaer{"ui", "shaders/ui_v_shader.glsl",
                     "shaders/ui_f_shader.glsl"};
    Shader text_shdaer{"text", "shaders/text_v_shader.glsl",
                       "shaders/text_f_shader.glsl"};
    Shader under_water_shader{"under_water",
                              "shaders/under_water_v_shader.glsl",
                              "shaders/under_water_f_shader.glsl"};

    m_shaders.insert({world_shader.hash(), std::move(world_shader)});
    m_shaders.insert({outline_shader.hash(), std::move(outline_shader)});
    m_shaders.insert({sky_shdaer.hash(), std::move(sky_shdaer)});
    m_shaders.insert({ui_shdaer.hash(), std::move(ui_shdaer)});
    m_shaders.insert({text_shdaer.hash(), std::move(text_shdaer)});
    m_shaders.insert(
        {under_water_shader.hash(), std::move(under_water_shader)});

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef DEBUG_MODE
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity,
           GLsizei length, const GLchar* message, const void* user_param) {
            Logger::log(Logger::Level::DEBUG, std::source_location::current(),
                        "GL Debug: {}", reinterpret_cast<const char*>(message));
        },
        nullptr);
#endif

    m_vao.resize(NUM_VAO);
    glGenVertexArrays(NUM_VAO, m_vao.data());
    glBindVertexArray(0);
    glGenBuffers(1, &m_outline_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_outline_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VER), CUBE_VER, GL_STATIC_DRAW);

    glGenBuffers(1, &m_outline_indices_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_outline_indices_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(OUTLINE_CUBE_INDICES),
                 OUTLINE_CUBE_INDICES, GL_STATIC_DRAW);
    glGenBuffers(1, &m_sky_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_sky_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES_POS), VERTICES_POS,
                 GL_STATIC_DRAW);

    glGenBuffers(1, &m_ui_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);

    for (int i = 0; i < 6; i++) {
        Vertex2D vex{SQUARE_VERTICES[i][0], SQUARE_VERTICES[i][1],
                     SQUARE_TEXTURE_POS[i][0], SQUARE_TEXTURE_POS[i][1], 0};
        m_ui.emplace_back(vex);
    }

    glBufferData(GL_ARRAY_BUFFER, m_ui.size() * sizeof(Vertex2D), m_ui.data(),
                 GL_STATIC_DRAW);

    glGenBuffers(1, &m_text_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_text_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    init_underwater();
    init_text();
    hot_reload();
}

const Shader& Renderer::get_shader(const std::string& name) const {
    auto it = m_shaders.find(HASH::str(name));
    ASSERT_MSG(it != m_shaders.end(), "Shader don't find, check the name");
    return it->second;
}

void Renderer::init_underwater() {
    glGenBuffers(1, &m_quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTICES), QUAD_VERTICES,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::init_text() {
    const auto& shader = get_shader("text");
    Text::set_loc(shader);
    DebugCollector::get().init_text();
}

void Renderer::render() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(m_vao[0]);
    render_sky();
    glBindVertexArray(m_vao[1]);
    render_world();
    glBindVertexArray(m_vao[2]);
    render_outline();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(m_vao[3]);
    render_underwater();
    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(m_vao[4]);
    render_ui();
    glBindVertexArray(m_vao[5]);
    render_text();
    glBindVertexArray(0);
    render_dev_panel();
}

void Renderer::render_outline() {
    const auto& shader = get_shader("outline");
    shader.use();

    m_mv_loc = shader.loc("mv_matrix");
    m_proj_loc = shader.loc("proj_matrix");

    const auto& block_pos = m_world.get_look_block_pos("TestPlayer");

    if (block_pos != std::nullopt) {
        m_m_mat =
            glm::translate(glm::mat4(1.0f), glm::vec3(block_pos.value().pos));
        m_mv_mat = m_v_mat * m_m_mat;
        glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_mv_mat));
        glUniformMatrix4fv(m_proj_loc, 1, GL_FALSE, glm::value_ptr(m_p_mat));

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

    const auto& shader = get_shader("sky");

    shader.use();
    m_mv_loc = shader.loc("mv_matrix");
    m_proj_loc = shader.loc("proj_matrix");

    m_m_mat = glm::translate(glm::mat4(1.0f), m_camera.get_camera_pos() -
                                                  glm::vec3(0.5f, 0.5f, 0.5f));
    m_v_mat = m_camera.get_camera_lookat();
    m_mv_mat = m_v_mat * m_m_mat;

    glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_mv_mat));
    glUniformMatrix4fv(m_proj_loc, 1, GL_FALSE, glm::value_ptr(m_p_mat));

    glBindBuffer(GL_ARRAY_BUFFER, m_sky_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::render_text() {
    const auto& shader = get_shader("text");
    shader.use();

    glDisable(GL_DEPTH_TEST);
    m_proj_loc = shader.loc("projection");

    glUniformMatrix4fv(m_proj_loc, 1, GL_FALSE, glm::value_ptr(m_ui_proj));

    auto& texts = DebugCollector::get().all_texts();
    for (auto& t : texts) {
        t.second.render();
    }

    glEnable(GL_DEPTH_TEST);
}

void Renderer::render_ui() {
    const auto& shader = get_shader("ui");
    shader.use();

    glDisable(GL_DEPTH_TEST);

    m_mv_loc = shader.loc("m_matrix");
    m_proj_loc = shader.loc("proj_matrix");

    glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_ui_m_matrix));
    glUniformMatrix4fv(m_proj_loc, 1, GL_FALSE, glm::value_ptr(m_ui_proj));

    glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          (void*)offsetof(Vertex2D, s));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          (void*)offsetof(Vertex2D, layer));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_manager.get_ui_array());

    glDrawArrays(GL_TRIANGLES, 0, 6);
    Tools::check_opengl_error();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnable(GL_DEPTH_TEST);
}

void Renderer::render_underwater() {
    const auto& shader = get_shader("under_water");
    shader.use();
    glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    glUniform1i(shader.loc("u_sceneTexture"), 0);
    glUniform1f(shader.loc("u_time"), glfwGetTime());
    glUniform1i(shader.loc("u_underwater"), m_camera.is_under_water());
    glUniform3f(shader.loc("u_waterColor"), 0.1f, 0.25f, 0.35f);
    glUniform1f(shader.loc("u_fogDensity"), 0.08f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_screen_texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::update(float delta_time) { m_delta_time = delta_time; }

void Renderer::update_fov(float fov) {
    m_fov = fov;
    m_p_mat = glm::perspective(glm::radians(fov), m_aspect, 0.1f, 1000.0f);
}

void Renderer::update_proj_matrix(float aspect, float width, float height) {
    m_aspect = aspect;
    m_p_mat = glm::perspective(glm::radians(m_fov), aspect, 0.1f, 1000.0f);
    m_ui_proj = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    // scale and then translate
    m_ui_m_matrix =
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(width / 2.0f, height / 2.0f, 0.0)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 50.0f, 1.0f));
}

void Renderer::updata_framebuffer(int width, int height) {
    if (width <= 0 || height <= 0)
        return;
    if (m_fbo == 0) {
        glGenFramebuffers(1, &m_fbo);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glDeleteTextures(1, &m_screen_texture);
    glDeleteRenderbuffers(1, &m_depth_render_buffer);

    glGenTextures(1, &m_screen_texture);
    glBindTexture(GL_TEXTURE_2D, m_screen_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_screen_texture, 0);

    glGenRenderbuffers(1, &m_depth_render_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depth_render_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, m_depth_render_buffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Logger::error("FBO incomplete after resize!");
    } else {
        Logger::info("Frame Buffer Complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::render_world() {
    const auto& shader = get_shader("world");
    shader.use();

    m_mv_loc = shader.loc("mv_matrix");
    m_proj_loc = shader.loc("proj_matrix");
    glActiveTexture(GL_TEXTURE0);

    m_m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    m_v_mat = m_camera.get_camera_lookat();
    m_mv_mat = m_v_mat * m_m_mat;
    glUniformMatrix4fv(m_mv_loc, 1, GL_FALSE, glm::value_ptr(m_mv_mat));
    glUniformMatrix4fv(m_proj_loc, 1, GL_FALSE, glm::value_ptr(m_p_mat));
    m_mvp_mat = m_p_mat * m_mv_mat;
    m_world.render(m_mvp_mat, m_texture_manager, m_camera.get_camera_pos());
}

void Renderer::render_dev_panel() {
    glDisable(GL_DEPTH_TEST);
    m_dev_panel.render();
    glEnable(GL_DEPTH_TEST);
}

} // namespace Cubed