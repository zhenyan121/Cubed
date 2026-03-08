#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Cubed/camera.hpp>
#include <Cubed/config.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/texture_manager.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/shader_tools.hpp>

constexpr int NUM_VAO = 1;
constexpr int NUM_VBO = 1;

struct Vertex {
    float x, y, z;
    float s, t;
    float layer;
};

GLuint rendering_program;
GLuint vao[NUM_VAO];
GLuint vbo[NUM_VBO];
GLuint mv_loc, proj_loc;
int width ,height;
float aspect;
glm::mat4 p_mat, v_mat, m_mat, mv_mat;
float inc = 0.01f;
float tf = 0.0f;
double last_time = 0.0f;
double delta_time = 0.0f;
GLuint texture_array;
Player player;
Camera camera;
TextureManager texture_manager;
World world;
std::vector<Vertex> vertex_data;


void setup_vertices(void) {
    float vertices_pos[6][6][3] = {
        // ===== front (z = +1) =====
        -0.5f, -0.5f,  0.5f,  // bottom left
        -0.5f,  0.5f,  0.5f,  // top left
        0.5f,  0.5f,  0.5f,  // top right
        0.5f,  0.5f,  0.5f,  // top right
        0.5f, -0.5f,  0.5f,  // bottom right
        -0.5f, -0.5f,  0.5f,  // bottom left
        // ===== right (x = +1) =====
        0.5f, -0.5f,  0.5f,  // bottom front
        0.5f, -0.5f, -0.5f,  // bottom back
        0.5f,  0.5f, -0.5f,  // top back
        0.5f,  0.5f, -0.5f,  // top back
        0.5f,  0.5f,  0.5f,  // top front
        0.5f, -0.5f,  0.5f,  // bottom front
        // ===== back (z = -1) =====
        -0.5f, -0.5f, -0.5f,  // bottom left
        0.5f, -0.5f, -0.5f,  // bottom right
        0.5f,  0.5f, -0.5f,  // top right
        0.5f,  0.5f, -0.5f,  // top right
        -0.5f,  0.5f, -0.5f,  // top left
        -0.5f, -0.5f, -0.5f,  // bottom left
        // ===== left (x = -1) =====
        -0.5f, -0.5f, -0.5f,  // bottom back
        -0.5f, -0.5f,  0.5f,  // bottom front
        -0.5f,  0.5f,  0.5f,  // top front
        -0.5f,  0.5f,  0.5f,  // top front
        -0.5f,  0.5f, -0.5f,  // top back
        -0.5f, -0.5f, -0.5f,  // bottom back
        // ===== top (y = +1) =====
        -0.5f,  0.5f, -0.5f,  // back left
        0.5f,  0.5f, -0.5f,  // back right
        0.5f,  0.5f,  0.5f,  // front right
        0.5f,  0.5f,  0.5f,  // front right
        -0.5f,  0.5f,  0.5f,  // front left
        -0.5f,  0.5f, -0.5f,  // back left
        // ===== bottom (y = -1) =====
        -0.5f, -0.5f,  0.5f,  // front left
        0.5f, -0.5f,  0.5f,  // front right
        0.5f, -0.5f, -0.5f,  // back right
        0.5f, -0.5f, -0.5f,  // back right
        -0.5f, -0.5f, -0.5f,  // back left
        -0.5f, -0.5f,  0.5f   // front left
    };

    float tex_coords[6][6][2] = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

    };

    // every block
    for (int x = 0; x < DISTANCE * CHUCK_SIZE; x++) {
        for (int z = 0; z < DISTANCE * CHUCK_SIZE; z++) {
            for (int y = 0; y < CHUCK_SIZE; y++) {
                const auto& block_render_data = world.get_block_render_data(x, y, z);
                // air
                if (block_render_data.block_id == 0) {
                    continue;
                }
                for (int face = 0; face < 6; face++) {
                    if (!block_render_data.draw_face[face]) {
                        continue;
                    }
                    for (int i = 0; i < 6; i++) {
                        Vertex vex = {
                            vertices_pos[face][i][0] + (float)x * 1.0f,
                            vertices_pos[face][i][1] + (float)y * 1.0f,
                            vertices_pos[face][i][2] + (float)z * 1.0f,
                            tex_coords[face][i][0],
                            tex_coords[face][i][1],
                            static_cast<float>(block_render_data.block_id * 6 + face) 

                        };
                        vertex_data.emplace_back(vex);
                    }    
                }
            }
            
        }
    }

    
    glGenVertexArrays(NUM_VAO, vao);
    glBindVertexArray(vao[0]);
    glGenBuffers(NUM_VBO, vbo);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(Vertex), vertex_data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

}




GLuint create_shader_program() {
    std::string v_shader_str = Shader::read_shader_source("shaders/vShader.glsl");
    std::string f_shader_str = Shader::read_shader_source("shaders/fShader.glsl");
    const char *v_shader_source = v_shader_str.c_str();
    const char *f_shader_source = f_shader_str.c_str();

    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);

    GLint vc, fc;
    glShaderSource(v_shader, 1, &v_shader_source, NULL);
    glShaderSource(f_shader, 1, &f_shader_source, NULL);
    glCompileShader(v_shader);
    Shader::check_opengl_error();
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &vc);
    if (vc != 1) {
        LOG::error("vertex compilation failed");
        Shader::print_shader_log(v_shader);
    }
    glCompileShader(f_shader);         
    Shader::check_opengl_error();
    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &fc);
    if (fc != 1) {
        LOG::error("vertex compilation failed");
        Shader::print_shader_log(f_shader);
    }
    GLuint vf_program = glCreateProgram();
    glAttachShader(vf_program, v_shader);
    glAttachShader(vf_program, f_shader);
    glLinkProgram(vf_program);

    GLint linked;
    Shader::check_opengl_error();
    glGetProgramiv(vf_program, GL_LINK_STATUS, &linked);
    if (linked != 1) {
        LOG::error("linking failed");
        Shader::print_program_info(vf_program);
    }

    return vf_program;
}


void init(GLFWwindow* window) {
    rendering_program = create_shader_program();

    mv_loc = glGetUniformLocation(rendering_program, "mv_matrix");
    proj_loc = glGetUniformLocation(rendering_program, "proj_matrix");

    camera.camera_init(&player);
    glfwGetFramebufferSize(window, &width, &height);
    aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    p_mat = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f); 

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    #ifndef NDEBUG    
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param) {
        LOG::info("GL Debug: {}", reinterpret_cast<const char*>(message));
    }, nullptr);
    #endif
    
    setup_vertices();

}

void window_reshape_callback(GLFWwindow* window, int new_width, int new_height) {
    glfwGetFramebufferSize(window, &width, &height);
    aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    p_mat = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
}




void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    camera.update_cursor_position_camera(xpos, ypos);
}



void display(GLFWwindow* window, double current_time) {
    delta_time = current_time - last_time;
    last_time = current_time;
    player.update(delta_time);
    camera.update_move_camera();


    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(rendering_program);
    glBindVertexArray(vao[0]);

    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, s));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, layer));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
    

    m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    v_mat = camera.get_camera_lookat();
    mv_mat = v_mat * m_mat;  
    glUniformMatrix4fv(mv_loc, 1, GL_FALSE, glm::value_ptr(mv_mat));
    glUniformMatrix4fv(proj_loc, 1 ,GL_FALSE, glm::value_ptr(p_mat));
    glDrawArrays(GL_TRIANGLES, 0, vertex_data.size() * 3);
    
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch(key) {
        case GLFW_KEY_ESCAPE:
            if (action == GLFW_PRESS) {
                
                if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            break;
            
    }

    player.update_player_move_state(key, action);
}


int main() {
    if (!glfwInit()) {
        LOG::error("glfwinit fail");
        exit(EXIT_FAILURE);        
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Cubed", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG::error("Failed to initialize glad");
        return -1;
    }

    LOG::info("OpenGL Version: {}.{}", GLVersion.major, GLVersion.minor);
    LOG::info("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    glfwSwapInterval(1);   
    glfwSetWindowSizeCallback(window, window_reshape_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    MapTable::init_map();
    texture_manager.init_texture();
    world.init_world();
    texture_array = texture_manager.get_texture_array();
    init(window);
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while(!glfwWindowShouldClose(window)) {
        display(window, glfwGetTime());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


