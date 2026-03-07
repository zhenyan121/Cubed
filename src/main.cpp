#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Cubed/camera.hpp>
#include <Cubed/config.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/texture_manager.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/shader_tools.hpp>

constexpr int NUM_VAO = 1;
constexpr int NUM_VBO = 3;
bool block_present[WORLD_SIZE_X][WORLD_SIZE_Z] = {false};
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
std::vector<GLuint> grass_block_texture(6);
Player player;
Camera camera;
TextureManager texture_manager;
void setup_vertices(void) {
    float vertices_pos[108] = {
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

    float tex_coords[72] {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

    };

    
    glGenVertexArrays(NUM_VAO, vao);
    glBindVertexArray(vao[0]);
    glGenBuffers(NUM_VBO, vbo);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_pos), vertices_pos, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}




GLuint create_shader_program() {
    std::string v_shader_str = read_shader_source("shaders/vShader.glsl");
    std::string f_shader_str = read_shader_source("shaders/fShader.glsl");
    const char *v_shader_source = v_shader_str.c_str();
    const char *f_shader_source = f_shader_str.c_str();

    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);

    GLint vc, fc;
    glShaderSource(v_shader, 1, &v_shader_source, NULL);
    glShaderSource(f_shader, 1, &f_shader_source, NULL);
    glCompileShader(v_shader);
    check_opengl_error();
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &vc);
    if (vc != 1) {
        LOG::error("vertex compilation failed");
        print_shader_log(v_shader);
    }
    glCompileShader(f_shader);         
    check_opengl_error();
    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &fc);
    if (fc != 1) {
        LOG::error("vertex compilation failed");
        print_shader_log(f_shader);
    }
    GLuint vf_program = glCreateProgram();
    glAttachShader(vf_program, v_shader);
    glAttachShader(vf_program, f_shader);
    glLinkProgram(vf_program);

    GLint linked;
    check_opengl_error();
    glGetProgramiv(vf_program, GL_LINK_STATUS, &linked);
    if (linked != 1) {
        LOG::error("linking failed");
        print_program_info(vf_program);
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
    
    grass_block_texture = texture_manager.get_block_texture("grass_block").texture;

    for (int i = 0; i < 6; i++) {
        glBindTexture(GL_TEXTURE_2D, grass_block_texture[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);    
    
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

    for (int i = 0; i < WORLD_SIZE_X; i++) {
        for (int j = 0; j < WORLD_SIZE_Z; j++) {
            block_present[i][j] = true;
        }
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(rendering_program);
    glBindVertexArray(vao[0]);

    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    for (int x = 0; x < WORLD_SIZE_X; x++) {
        for (int z = 0; z < WORLD_SIZE_Z; z++) {

            int wx = x - WORLD_SIZE_X / 2;
            int wz = z - WORLD_SIZE_Z / 2;

            m_mat = glm::translate(glm::mat4(1.0f), glm::vec3((float)wx, 0.0f, (float)wz));
            v_mat = camera.get_camera_lookat();
            mv_mat = v_mat * m_mat;
            glUniformMatrix4fv(mv_loc, 1, GL_FALSE, glm::value_ptr(mv_mat));
            glUniformMatrix4fv(proj_loc, 1 ,GL_FALSE, glm::value_ptr(p_mat));

            bool draw_face[6] = {true, true, true, true, true, true};

            if (z < WORLD_SIZE_Z - 1&& block_present[x][z + 1]) {
                draw_face[0] = false;
            }
            if (x < WORLD_SIZE_X - 1 && block_present[x + 1][z]) {
                draw_face[1] = false;
            }
            if (z > 0 && block_present[x][z - 1]) {
                draw_face[2] = false;
            }
            if (x > 0 && block_present[x - 1][z]) {
                draw_face[3] = false;
            }


            for (int face = 0; face < 6; face++) {
                if (!draw_face[face]) {
                    continue;
                }
                glBindTexture(GL_TEXTURE_2D, grass_block_texture[face]);
                

                //glPointSize(30.0f);
                
                glDrawArrays(GL_TRIANGLES, face * 6, 6);
            }
        }
    }
    
    
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


