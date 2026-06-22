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
#include "Cubed/tools/math_tools.hpp"
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
    glDeleteTextures(1, &m_screen_depth_texture);

    glDeleteFramebuffers(1, &m_oit_fbo);
    glDeleteTextures(1, &m_accum_texture);
    glDeleteTextures(1, &m_reveal_texture);
    glDeleteTextures(1, &m_oit_depth_texture);

    glDeleteFramebuffers(1, &m_depth_map_fbo);
    glDeleteTextures(1, &m_depth_map_texture);
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

    Shader world_shader{"normal_block", "shaders/block_v_shader.glsl",
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
    Shader accum_shader{"accum", "shaders/block_accumulation_v_shader.glsl",
                        "shaders/block_accumulation_f_shader.glsl"};
    Shader composite_block_shader{"composite",
                                  "shaders/block_composite_v_shader.glsl",
                                  "shaders/block_composite_f_shader.glsl"};
    Shader depth_shader{"depth_shader", "shaders/depth_shader.glsl",
                        "shaders/depth_fragment_shader.glsl"};
    Shader billboard{"billboard", "shaders/billboard_v_shader.glsl",
                     "shaders/billboard_f_shader.glsl"};
    Shader water_shader{"water", "shaders/water_v_shader.glsl",
                        "shaders/water_f_shader.glsl"};

    m_shaders.insert({world_shader.hash(), std::move(world_shader)});
    m_shaders.insert({outline_shader.hash(), std::move(outline_shader)});
    m_shaders.insert({sky_shdaer.hash(), std::move(sky_shdaer)});
    m_shaders.insert({ui_shdaer.hash(), std::move(ui_shdaer)});
    m_shaders.insert({text_shdaer.hash(), std::move(text_shdaer)});
    m_shaders.insert(
        {under_water_shader.hash(), std::move(under_water_shader)});
    m_shaders.insert({accum_shader.hash(), std::move(accum_shader)});
    m_shaders.insert(
        {composite_block_shader.hash(), std::move(composite_block_shader)});
    m_shaders.insert({depth_shader.hash(), std::move(depth_shader)});
    m_shaders.insert({billboard.hash(), std::move(billboard)});
    m_shaders.insert({water_shader.hash(), std::move(water_shader)});
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

    glBindVertexArray(m_vao[2]);
    glGenBuffers(1, &m_outline_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_outline_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VER), CUBE_VER, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &m_outline_indices_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_outline_indices_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(OUTLINE_CUBE_INDICES),
                 OUTLINE_CUBE_INDICES, GL_STATIC_DRAW);

    glBindVertexArray(m_vao[1]);
    glGenBuffers(1, &m_sky_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_sky_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES_POS), VERTICES_POS,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(m_vao[3]);
    glGenBuffers(1, &m_ui_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);

    for (int i = 0; i < 6; i++) {
        Vertex2D vex{SQUARE_VERTICES[i][0], SQUARE_VERTICES[i][1],
                     SQUARE_TEXTURE_POS[i][0], SQUARE_TEXTURE_POS[i][1], 0};
        m_ui.emplace_back(vex);
    }

    glBufferData(GL_ARRAY_BUFFER, m_ui.size() * sizeof(Vertex2D), m_ui.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          (void*)offsetof(Vertex2D, s));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          (void*)offsetof(Vertex2D, layer));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    init_quad();
    init_text();
    hot_reload();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

const Shader& Renderer::get_shader(const std::string& name) const {
    auto it = m_shaders.find(HASH::str(name));
    ASSERT_MSG(it != m_shaders.end(), "Shader don't find, check the name");
    return it->second;
}

void Renderer::init_quad() {
    glBindVertexArray(m_vao[0]);
    glGenBuffers(1, &m_quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTICES), QUAD_VERTICES,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));
}

void Renderer::init_text() {
    glBindVertexArray(m_vao[4]);
    glGenBuffers(1, &m_text_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_text_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    const auto& shader = get_shader("text");
    Text::set_loc(shader);
    DebugCollector::get().init_text();
}

void Renderer::render() {
    glDisable(GL_FRAMEBUFFER_SRGB);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    day_night_calculation();
    render_sky();
    render_world();
    render_outline();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    render_underwater();
    glDisable(GL_FRAMEBUFFER_SRGB);
    render_ui();
    render_text();
    render_dev_panel();
}

void Renderer::day_night_calculation() {
    m_parallel_light.sundir = glm::normalize(m_world.sunlight_dir());
    m_parallel_light.sun_height = (-m_parallel_light.sundir).y;
    m_parallel_light.lightdir = m_parallel_light.sundir;

    m_parallel_light.day_light =
        glm::smoothstep(0.15f, 0.3f, m_parallel_light.sun_height);

    m_parallel_light.sun_color = mix(SUNSET_SUNLIGHT_COLOR, NOON_SUNLIGHT_COLOR,
                                     m_parallel_light.day_light);

    glm::vec3 ambient_color = mix(SUNSET_AMBIENT_COLOR, NOON_AMBIENT_COLOR,
                                  m_parallel_light.day_light);

    m_parallel_light.day_factor =
        glm::smoothstep(-0.15f, 0.05f, m_parallel_light.sun_height);
    auto day_factor = m_parallel_light.day_factor;
    float light_intensity =
        glm::smoothstep(moon_intensity, sun_intensity, day_factor);
    m_parallel_light.directional_light_color =
        glm::mix(MOON_COLOR, m_parallel_light.sun_color, day_factor) *
        light_intensity;
    m_parallel_light.finnal_ambient_color =
        glm::mix(NIGHT_AMBIENT_COLOR, ambient_color, day_factor);

    m_ambient_strength = glm::mix(0.45f, 0.25f, day_factor);
}

void Renderer::render_outline() {
    const auto& shader = get_shader("outline");
    shader.use();

    const auto& block_pos = m_world.get_look_block_pos("TestPlayer");

    if (block_pos != std::nullopt) {

        m_m_mat =
            glm::translate(glm::mat4(1.0f), glm::vec3(block_pos.value().pos));

        m_v_mat = m_camera.get_camera_lookat();
        m_mv_mat = m_v_mat * m_m_mat;

        shader.set_loc("mv_matrix", m_mv_mat);
        shader.set_loc("proj_matrix", m_p_mat);

        glBindVertexArray(m_vao[2]);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glLineWidth(4.0f);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }
}

void Renderer::render_sky() {

    glm::vec3 zenith = {0.20f, 0.45f, 0.95f};

    glm::vec3 horizon = {0.55f, 0.75f, 1.00f};

    glm::vec3 sunset_zenith = {0.05f, 0.10f, 0.25f};

    glm::vec3 sunset_horizon = {1.0f, 0.35f, 0.10f};

    glm::vec3 night_zenith = {0.018f, 0.023f, 0.048f};
    glm::vec3 night_horizon = {0.022f, 0.027f, 0.052f};

    constexpr float NIGHT_SHARPNESS = 0.35f;
    constexpr float SUNSET_SHARPNESS = 0.6f;
    constexpr float NOON_SHARPNESS = 0.35f;

    constexpr float NIGHT_CLOUD_MIX = 0.3f;
    constexpr float SUNSET_CLOUD_MIX = 0.4f;
    constexpr float NOON_CLOUD_MIX = 0.7;

    glm::vec3 day_top = mix(sunset_zenith, zenith, m_parallel_light.day_light);
    glm::vec3 day_bottom =
        mix(sunset_horizon, horizon, m_parallel_light.day_light);

    m_sky_uniform.sky_top =
        mix(night_zenith, day_top, m_parallel_light.day_factor);
    m_sky_uniform.sky_bottom =
        mix(night_horizon, day_bottom, m_parallel_light.day_factor);

    float day_sharpness =
        glm::mix(SUNSET_SHARPNESS, NOON_SHARPNESS, m_parallel_light.day_light);

    m_sky_uniform.horizon_sharpness =
        glm::mix(NIGHT_SHARPNESS, day_sharpness, m_parallel_light.day_factor);

    float day_cloud_mix =
        glm::mix(SUNSET_CLOUD_MIX, NOON_CLOUD_MIX, m_parallel_light.day_light);
    m_sky_uniform.cloud_white_mix =
        glm::mix(NIGHT_CLOUD_MIX, day_cloud_mix, m_parallel_light.day_factor);

    m_cloud_time += m_delta_time * m_cloud_speed;

    const auto& sky_shader = get_shader("sky");

    sky_shader.use();

    m_m_mat = glm::translate(glm::mat4(1.0f), m_camera.get_camera_pos() -
                                                  glm::vec3(0.5f, 0.5f, 0.5f));
    m_v_mat = m_camera.get_camera_lookat();
    m_mv_mat = m_v_mat * m_m_mat;

    m_sky_uniform.sun_dir_view = (-m_parallel_light.sundir);
    sky_shader.set_loc("mv_matrix", m_mv_mat);
    sky_shader.set_loc("proj_matrix", m_p_mat);
    sky_shader.set_loc("skyTop", m_sky_uniform.sky_top);
    sky_shader.set_loc("skyBottom", m_sky_uniform.sky_bottom);
    sky_shader.set_loc("sunDir", m_sky_uniform.sun_dir_view);
    sky_shader.set_loc("sunColor", m_parallel_light.directional_light_color);
    sky_shader.set_loc("horizonSharpness", m_sky_uniform.horizon_sharpness);
    sky_shader.set_loc("time", m_cloud_time);
    sky_shader.set_loc("cloudWhiteMix", m_sky_uniform.cloud_white_mix);
    sky_shader.set_loc("cloudThresholdLow", m_cloud_threshold_low);
    sky_shader.set_loc("cloudThresholdHigh", m_cloud_threshold_high);
    glBindVertexArray(m_vao[1]);

    glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glEnable(GL_DEPTH_TEST);

    // draw sun and moon
    const auto& billboard = get_shader("billboard");
    billboard.use();
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_vao[0]);
    // draw sun
    glm::vec3 sun_pos = m_camera.get_camera_pos() +
                        normalize(-m_world.sunlight_dir()) * (FAR_PLANE * 0.9f);
    glm::vec3 sun_view_pos = glm::vec3(m_v_mat * glm::vec4(sun_pos, 1.0f));
    m_mv_mat = glm::translate(glm::mat4(1.0f), sun_view_pos) *
               glm::scale(glm::mat4(1.0f), glm::vec3(SUN_SIZE)) *
               glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, 0.0f));

    billboard.set_loc("mv_matrix", m_mv_mat);
    billboard.set_loc("proj_matrix", m_p_mat);
    billboard.set_loc("color", SUN_COLOR);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glm::vec3 moon_pos = m_camera.get_camera_pos() +
                         normalize(m_world.sunlight_dir()) * (FAR_PLANE * 0.9f);
    glm::vec3 moon_view_pos = glm::vec3(m_v_mat * glm::vec4(moon_pos, 1.0f));
    m_mv_mat = glm::translate(glm::mat4(1.0f), moon_view_pos) *
               glm::scale(glm::mat4(1.0f), glm::vec3(MOON_SIZE)) *
               glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, 0.0f));
    billboard.set_loc("mv_matrix", m_mv_mat);
    billboard.set_loc("proj_matrix", m_p_mat);
    billboard.set_loc("color", MOON_COLOR);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDepthMask(GL_TRUE);
}

void Renderer::render_text() {

    glBindVertexArray(m_vao[4]);

    const auto& shader = get_shader("text");

    shader.use();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    shader.set_loc("projection", m_ui_proj);

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.set_loc("m_matrix", m_ui_m_matrix);
    shader.set_loc("proj_matrix", m_ui_proj);

    glBindVertexArray(m_vao[3]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_manager.get_ui_array());

    glDrawArrays(GL_TRIANGLES, 0, 6);
    Tools::check_opengl_error();

    glEnable(GL_DEPTH_TEST);
}

void Renderer::render_underwater() {
    const auto& shader = get_shader("under_water");
    shader.use();

    glBindVertexArray(m_vao[0]);

    shader.set_loc("u_sceneTexture", 0);
    shader.set_loc("u_time", static_cast<float>(glfwGetTime()));
    shader.set_loc("u_underwater", m_camera.is_under_water());
    shader.set_loc("u_waterColor", glm::vec3(0.1f, 0.25f, 0.35f));
    shader.set_loc("u_fogDensity", m_underwater_fog_density);
    shader.set_loc("cameraPos", m_camera.get_camera_pos());
    shader.set_loc("sunDir", -m_parallel_light.sundir);
    shader.set_loc("waterDensity", m_water_density);
    shader.set_loc("InverseViewProjection", glm::inverse(m_p_mat * m_v_mat));
    shader.set_loc("sunColor", m_parallel_light.sun_color);
    shader.set_loc("u_lightSpaceMatrix", m_parallel_light.light_space_matrix);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_screen_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_screen_depth_texture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_depth_map_texture);
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
    m_p_mat =
        glm::perspective(glm::radians(m_fov), aspect, NEAR_PLANE, FAR_PLANE);
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
    if (m_oit_fbo == 0) {
        glGenFramebuffers(1, &m_oit_fbo);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glDeleteTextures(1, &m_screen_texture);
    glDeleteTextures(1, &m_screen_depth_texture);

    glGenTextures(1, &m_screen_texture);
    glBindTexture(GL_TEXTURE_2D, m_screen_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_screen_texture, 0);

    glGenTextures(1, &m_screen_depth_texture);
    glBindTexture(GL_TEXTURE_2D, m_screen_depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           m_screen_depth_texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Logger::error("FBO incomplete after resize!");
    } else {
        Logger::info("Frame Buffer Complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_oit_fbo);
    glDeleteTextures(1, &m_accum_texture);
    glDeleteTextures(1, &m_reveal_texture);
    glDeleteTextures(1, &m_oit_depth_texture);
    glGenTextures(1, &m_accum_texture);
    glBindTexture(GL_TEXTURE_2D, m_accum_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
                 GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_accum_texture, 0);
    glGenTextures(1, &m_reveal_texture);
    glBindTexture(GL_TEXTURE_2D, m_reveal_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED,
                 GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           m_reveal_texture, 0);
    glGenTextures(1, &m_oit_depth_texture);
    glBindTexture(GL_TEXTURE_2D, m_oit_depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           m_oit_depth_texture, 0);
    GLenum draw_buffer[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, draw_buffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Logger::error("FBO incomplete after resize!");
    } else {
        Logger::info("Frame Buffer Complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // depth map fbo
    if (m_depth_map_fbo == 0) {
        glGenFramebuffers(1, &m_depth_map_fbo);
    }
    glDeleteTextures(1, &m_depth_map_texture);
    glGenTextures(1, &m_depth_map_texture);

    glBindTexture(GL_TEXTURE_2D, m_depth_map_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, DEPTH_MAP_SIZE,
                 DEPTH_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    // Manually compare shadows
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
    //                 GL_COMPARE_REF_TO_TEXTURE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindFramebuffer(GL_FRAMEBUFFER, m_depth_map_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           m_depth_map_texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Logger::error("FBO incomplete after resize!");
    } else {
        Logger::info("Frame Buffer Complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_width = width;
    m_height = height;
}
#pragma region render_world
void Renderer::render_world() {
    // shader map
    glm::mat4& light_space_matrix = m_parallel_light.light_space_matrix;
    auto& m_render_snapshots = m_world.render_snapshots();
    auto& camera_pos = m_camera.get_camera_pos();
    float texels_per_unit = 0.0f;

    const auto& lightdir = m_parallel_light.lightdir;

    if (m_shader_on) {
        const auto& depth_shader = get_shader("depth_shader");
        depth_shader.use();

        glm::vec3 cam_pos = m_camera.get_camera_pos();
        glm::vec3 cam_fwd = m_camera.get_camera_front();
        float half_extent = 128.0f;

        glm::vec3 center = cam_pos + cam_fwd * (half_extent * 0.5f);

        glm::vec3 raw_shadow_lightdir =
            quantize_sun_direction(lightdir, ANGLE_STEP_DEG);
        glm::vec3 shadow_lightdir =
            get_smoothed_shadow_lightdir(raw_shadow_lightdir, m_delta_time);
        glm::vec3 up = fabs(shadow_lightdir.y) > 0.99f ? glm::vec3(0, 0, 1)
                                                       : glm::vec3(0, 1, 0);

        glm::mat4 light_basis =
            glm::lookAt(glm::vec3(0.0f), shadow_lightdir, up);
        texels_per_unit = DEPTH_MAP_SIZE / (half_extent * 2.0f);
        glm::vec3 ls_center = glm::vec3(light_basis * glm::vec4(center, 1.0f));
        ls_center.x =
            std::round(ls_center.x * texels_per_unit) / texels_per_unit;
        ls_center.y =
            std::round(ls_center.y * texels_per_unit) / texels_per_unit;
        glm::vec3 snapped_center =
            glm::vec3(glm::inverse(light_basis) * glm::vec4(ls_center, 1.0f));

        float distance = half_extent * 1.5f;
        float near_plane = 1.0f;
        float far_plane = distance + half_extent * 2.0f;
        glm::vec3 light_pos = snapped_center - shadow_lightdir * distance;
        glm::mat4 light_view = glm::lookAt(light_pos, snapped_center, up);
        glm::mat4 light_projection =
            glm::ortho(-half_extent, half_extent, -half_extent, half_extent,
                       near_plane, far_plane);

        light_space_matrix = light_projection * light_view;
        depth_shader.set_loc("lightSpaceMatrix", light_space_matrix);
        depth_shader.set_loc("is_discard_tranparent", m_discard_tranparent);

        glViewport(0, 0, DEPTH_MAP_SIZE, DEPTH_MAP_SIZE);
        if (m_light_cull_face == 0) {
            glCullFace(GL_FRONT);
        } else if (m_light_cull_face == 1) {
            glCullFace(GL_BACK);
        } else {
            Logger::warn("Light Cull Face {} Over The Max Selection",
                         m_light_cull_face);
            glCullFace(GL_BACK);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_depth_map_fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_DEPTH_TEST);
        for (const auto& snapshot : m_render_snapshots) {
            glBindTexture(GL_TEXTURE_2D_ARRAY,
                          m_texture_manager.get_texture_array());
            glBindVertexArray(snapshot.normal_vao);

            glDrawArrays(GL_TRIANGLES, 0, snapshot.normal_vertices_count);
        }

        // cross_plane and discard

        for (const auto& snapshot : m_render_snapshots) {

            glm::vec2 camera_pos_xz{camera_pos.x, camera_pos.z};
            if (snapshot.cross_vertices_count != 0) {
                glm::vec2 center_xz{snapshot.center.x, snapshot.center.z};
                float dist2d = glm::distance(camera_pos_xz, center_xz);
                if (dist2d <= CROSS_PLANE_DISTANCE * 16) {
                    glBindTexture(GL_TEXTURE_2D_ARRAY,
                                  m_texture_manager.get_cross_plane_array());
                    glBindVertexArray(snapshot.cross_vao);

                    glDrawArrays(GL_TRIANGLES, 0,
                                 snapshot.cross_vertices_count);
                }
            }
            if (snapshot.normal_discard_vertices_count != 0) {
                glBindTexture(GL_TEXTURE_2D_ARRAY,
                              m_texture_manager.get_texture_array());
                glBindVertexArray(snapshot.normal_discard_vao);

                glDrawArrays(GL_TRIANGLES, 0,
                             snapshot.normal_discard_vertices_count);
            }
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glCullFace(GL_BACK);
    glViewport(0, 0, m_width, m_height);
    const auto& normal_block_shader = get_shader("normal_block");
    normal_block_shader.use();

    m_m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    m_v_mat = m_camera.get_camera_lookat();
    m_mv_mat = m_v_mat * m_m_mat;
    m_norm_mat = glm::transpose(glm::inverse(m_mv_mat));
    glm::vec3 light_dir_view = glm::normalize(glm::mat3(m_v_mat) * lightdir);
    normal_block_shader.set_loc("model_matrix", m_m_mat);
    normal_block_shader.set_loc("mv_matrix", m_mv_mat);
    normal_block_shader.set_loc("proj_matrix", m_p_mat);
    normal_block_shader.set_loc("norm_matrix", m_norm_mat);
    normal_block_shader.set_loc("lightSpaceMatrix", light_space_matrix);
    normal_block_shader.set_loc("ambientStrength", m_ambient_strength);
    normal_block_shader.set_loc("sunlightColor",
                                m_parallel_light.directional_light_color);
    normal_block_shader.set_loc("ambientColor",
                                m_parallel_light.finnal_ambient_color);
    normal_block_shader.set_loc("sunlightDir", light_dir_view);
    normal_block_shader.set_loc("shadowMode", m_shadow_mode);
    normal_block_shader.set_loc("shader_on", m_shader_on);
    normal_block_shader.set_loc("lightSizeUV",
                                static_cast<float>(m_light_size_uv));
    normal_block_shader.set_loc("minRadius", m_min_radius);
    normal_block_shader.set_loc("maxRadius", m_max_radius);
    normal_block_shader.set_loc("samples", m_samples);
    normal_block_shader.set_loc("specularStrength", m_specular_strength);
    normal_block_shader.set_loc("cameraPos", m_camera.get_camera_pos());
    normal_block_shader.set_loc("flipY", m_flip_y);
    normal_block_shader.set_loc("renderDistance", m_world.rendering_distance());
    normal_block_shader.set_loc("skyColor", m_sky_uniform.sky_top);
    m_mvp_mat = m_p_mat * m_mv_mat;

    auto& m_planes = m_world.planes();

    Math::extract_frustum_planes(m_mvp_mat, m_planes);

    int rendered_sum = 0;
    glEnable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_depth_map_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_manager.get_texture_array());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_manager.get_pbr_texture());
    normal_block_shader.set_loc("enablePBR", m_pbr);
    for (const auto& snapshot : m_render_snapshots) {

        if (Math::is_aabb_in_frustum(snapshot.center, snapshot.half_extents,
                                     m_planes)) {

            glBindVertexArray(snapshot.normal_vao);

            glDrawArrays(GL_TRIANGLES, 0, snapshot.normal_vertices_count);

            rendered_sum++;
        }
    }
    // discard
    for (const auto& snapshot : m_render_snapshots) {
        if (!Math::is_aabb_in_frustum(snapshot.center, snapshot.half_extents,
                                      m_planes)) {
            continue;
        }
        if (snapshot.normal_discard_vertices_count != 0) {
            glBindVertexArray(snapshot.normal_discard_vao);

            glDrawArrays(GL_TRIANGLES, 0,
                         snapshot.normal_discard_vertices_count);
        }
    }
    // cross_plane
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY,
                  m_texture_manager.get_cross_plane_array());
    normal_block_shader.set_loc("enablePBR", false);
    for (const auto& snapshot : m_render_snapshots) {
        if (!Math::is_aabb_in_frustum(snapshot.center, snapshot.half_extents,
                                      m_planes)) {
            continue;
        }
        glm::vec2 camera_pos_xz{camera_pos.x, camera_pos.z};
        if (snapshot.cross_vertices_count != 0) {
            glm::vec2 center_xz{snapshot.center.x, snapshot.center.z};
            float dist2d = glm::distance(camera_pos_xz, center_xz);
            if (dist2d <= CROSS_PLANE_DISTANCE * 16) {
                glBindVertexArray(snapshot.cross_vao);

                glDrawArrays(GL_TRIANGLES, 0, snapshot.cross_vertices_count);
            }
        }
    }

    // copy depth buffer

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_oit_fbo);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_oit_fbo);

    // pass one accumulate
    glBindFramebuffer(GL_FRAMEBUFFER, m_oit_fbo);

    glClearBufferfv(GL_COLOR, 0, glm::value_ptr(glm::vec4(0.0f)));
    float one = 1.0f;
    glClearBufferfv(GL_COLOR, 1, &one);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunci(0, GL_ONE, GL_ONE);

    glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

    auto set_accum_loc = [&](const Shader& accum_shader) {
        accum_shader.set_loc("mv_matrix", m_mv_mat);
        accum_shader.set_loc("proj_matrix", m_p_mat);
        accum_shader.set_loc("norm_matrix", m_norm_mat);
        accum_shader.set_loc("ambientStrength", m_ambient_strength);
        accum_shader.set_loc("sunlightColor",
                             m_parallel_light.directional_light_color);
        accum_shader.set_loc("ambientColor",
                             m_parallel_light.finnal_ambient_color);
        accum_shader.set_loc("sunlightDir", light_dir_view);
        accum_shader.set_loc("shader_on", m_shader_on);
        accum_shader.set_loc("specularStrength", m_specular_strength);
    };

    auto& accum_shader = get_shader("accum");
    accum_shader.use();

    set_accum_loc(accum_shader);
    accum_shader.set_loc("cameraPos", m_camera.get_camera_pos());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_manager.get_texture_array());
    for (const auto& snapshot : m_render_snapshots) {
        if (!Math::is_aabb_in_frustum(snapshot.center, snapshot.half_extents,
                                      m_planes)) {
            continue;
        }

        if (snapshot.normal_blend_vertices_count != 0) {

            glBindVertexArray(snapshot.normal_blend_vao);

            glDrawArrays(GL_TRIANGLES, 0, snapshot.normal_blend_vertices_count);
        }
    }

    // use SSR

    auto& water_shader = get_shader("water");
    water_shader.use();

    set_accum_loc(water_shader);

    water_shader.set_loc("sceneColorTex", 1);
    water_shader.set_loc("sceneDepthTex", 2);
    water_shader.set_loc("inv_proj_matrix", glm::inverse(m_p_mat));
    water_shader.set_loc("inv_view_matrix", glm::inverse(m_v_mat));

    // sky loc

    water_shader.set_loc("skyTop", m_sky_uniform.sky_top);
    water_shader.set_loc("skyBottom", m_sky_uniform.sky_bottom);
    water_shader.set_loc("sunDir", m_sky_uniform.sun_dir_view);
    water_shader.set_loc("sunColor", m_parallel_light.directional_light_color);
    water_shader.set_loc("horizonSharpness", m_sky_uniform.horizon_sharpness);
    water_shader.set_loc("time", glfwGetTime());
    water_shader.set_loc("cloudWhiteMix", m_sky_uniform.cloud_white_mix);
    water_shader.set_loc("cloudThresholdLow", m_cloud_threshold_low);
    water_shader.set_loc("cloudThresholdHigh", m_cloud_threshold_high);
    water_shader.set_loc("underwater", m_camera.is_under_water());
    water_shader.set_loc("refractStrength", m_refract_strength);
    water_shader.set_loc("enablePerturb", m_water_perturb);
    water_shader.set_loc("enableDepthFade", m_water_depth_fade);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_screen_texture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_screen_depth_texture);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_manager.get_texture_array());
    for (const auto& snapshot : m_render_snapshots) {
        if (!Math::is_aabb_in_frustum(snapshot.center, snapshot.half_extents,
                                      m_planes)) {
            continue;
        }

        if (snapshot.water_vertices_count != 0) {

            glBindVertexArray(snapshot.water_vao);

            glDrawArrays(GL_TRIANGLES, 0, snapshot.water_vertices_count);
        }
    }

    auto& composite_shader = get_shader("composite");
    glDisable(GL_BLEND);

    composite_shader.use();
    composite_shader.set_loc("u_accumTex", 0);
    composite_shader.set_loc("u_revealTex", 1);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vao[0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_accum_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_reveal_texture);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    DebugCollector::get().report(
        "rendered_chunk", "Rendered Chunk: " + std::to_string(rendered_sum));
}
#pragma endregion
void Renderer::render_dev_panel() {
    glDisable(GL_DEPTH_TEST);
    m_dev_panel.render();
    glEnable(GL_DEPTH_TEST);
}

glm::vec3 Renderer::quantize_sun_direction(const glm::vec3& lightdir,
                                           float angle_step_deg) const {
    float elevation = std::asin(glm::clamp(lightdir.y, -1.0f, 1.0f));
    float azimuth = std::atan2(lightdir.z, lightdir.x);

    float step = glm::radians(angle_step_deg);

    float quantized_elevation = std::round(elevation / step) * step;
    float quantized_azimuth = std::round(azimuth / step) * step;

    glm::vec3 quantized_dir;
    quantized_dir.x =
        std::cos(quantized_elevation) * std::cos(quantized_azimuth);
    quantized_dir.z =
        std::cos(quantized_elevation) * std::sin(quantized_azimuth);
    quantized_dir.y = std::sin(quantized_elevation);

    return glm::normalize(quantized_dir);
}

glm::vec3
Renderer::get_smoothed_shadow_lightdir(const glm::vec3& raw_shadow_lightdir,
                                       float dt) {
    if (!m_blend_initialized) {

        m_blend_from_lightdir = raw_shadow_lightdir;
        m_blend_to_lightdir = raw_shadow_lightdir;
        m_blend_t = 1.0f;
        m_blend_initialized = true;
        return raw_shadow_lightdir;
    }

    if (raw_shadow_lightdir != m_blend_to_lightdir) {
        glm::vec3 current_displayed = glm::normalize(
            Math::slerp(m_blend_from_lightdir, m_blend_to_lightdir, m_blend_t));

        m_blend_from_lightdir = current_displayed;
        m_blend_to_lightdir = raw_shadow_lightdir;
        m_blend_t = 0.0f;
    }

    m_blend_t = glm::min(m_blend_t + dt / BLEND_DURATION, 1.0f);

    return glm::normalize(
        Math::slerp(m_blend_from_lightdir, m_blend_to_lightdir, m_blend_t));
}

float& Renderer::ambient_strength() { return m_ambient_strength; }
bool& Renderer::discard_transparent() { return m_discard_tranparent; }
bool& Renderer::shader_on() { return m_shader_on; }
bool& Renderer::water_perturb() { return m_water_perturb; }
bool& Renderer::water_depth_fade() { return m_water_depth_fade; }
bool& Renderer::pbr() { return m_pbr; }
bool& Renderer::flip_y() { return m_flip_y; }
int& Renderer::shadow_mode() { return m_shadow_mode; }
int& Renderer::light_cull_face() { return m_light_cull_face; }
int& Renderer::light_size_uv() { return m_light_size_uv; }
float& Renderer::min_radius() { return m_min_radius; }
float& Renderer::max_radius() { return m_max_radius; }
int& Renderer::samples() { return m_samples; }
float& Renderer::specular_strength() { return m_specular_strength; }
float& Renderer::cloud_speed() { return m_cloud_speed; }
float& Renderer::cloud_threshold_low() { return m_cloud_threshold_low; }
float& Renderer::cloud_threshold_high() { return m_cloud_threshold_high; }
float& Renderer::refract_strength() { return m_refract_strength; }
float& Renderer::underwater_fog_density() { return m_underwater_fog_density; }
float& Renderer::water_density() { return m_water_density; }
} // namespace Cubed