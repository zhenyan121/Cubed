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

GLuint rendering_program, outline_program;
GLuint vao[NUM_VAO];
GLuint mv_loc, proj_loc;
int width ,height;
float aspect;
glm::mat4 p_mat, v_mat, m_mat, mv_mat;
float inc = 0.01f;
float tf = 0.0f;
double last_time = 0.0f;
double delta_time = 0.0f;
GLuint texture_array;
Camera camera;
TextureManager texture_manager;
World world;

GLuint outline_vbo, outline_indices_vbo;

void setup_vertices(void) {

    glGenVertexArrays(NUM_VAO, vao);
    glBindVertexArray(vao[0]);
    glBindVertexArray(0);
    glGenBuffers(1, &outline_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, outline_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VER), CUBE_VER, GL_STATIC_DRAW);
    glGenBuffers(1, &outline_indices_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outline_indices_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(OUTLINE_CUBE_INDICES), OUTLINE_CUBE_INDICES, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void init(GLFWwindow* window) {
    rendering_program = Shader::create_shader_program("shaders/vShader.glsl", "shaders/fShader.glsl");
    outline_program = Shader::create_shader_program("shaders/outline_v_shader.glsl", "shaders/outline_f_shader.glsl");


    camera.camera_init(&world.get_player("TestPlayer"));
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
    world.update(delta_time);
    camera.update_move_camera();


    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(rendering_program);
    glBindVertexArray(vao[0]);
    mv_loc = glGetUniformLocation(rendering_program, "mv_matrix");
    proj_loc = glGetUniformLocation(rendering_program, "proj_matrix");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
    m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    v_mat = camera.get_camera_lookat();
    mv_mat = v_mat * m_mat;  
    glUniformMatrix4fv(mv_loc, 1, GL_FALSE, glm::value_ptr(mv_mat));
    glUniformMatrix4fv(proj_loc, 1 ,GL_FALSE, glm::value_ptr(p_mat));

    world.render();
    
    glUseProgram(outline_program);
    mv_loc = glGetUniformLocation(outline_program, "mv_matrix");
    proj_loc = glGetUniformLocation(outline_program, "proj_matrix");

    m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(world.get_last_block_pos()));
    mv_mat = v_mat * m_mat;
    glUniformMatrix4fv(mv_loc, 1, GL_FALSE, glm::value_ptr(mv_mat));
    glUniformMatrix4fv(proj_loc, 1 ,GL_FALSE, glm::value_ptr(p_mat));

    glBindBuffer(GL_ARRAY_BUFFER, outline_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outline_indices_vbo);
    glLineWidth(5.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);


    
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch(key) {
        case GLFW_KEY_Q:
            if (action == GLFW_PRESS) {
                
                if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            break;
        case GLFW_KEY_ESCAPE:
            if (action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            
    }

    world.get_player("TestPlayer").update_player_move_state(key, action);
}


int main() {
    if (!glfwInit()) {
        LOG::error("glfwinit fail");
        exit(EXIT_FAILURE);        
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &outline_vbo);
    glDeleteBuffers(1, &outline_indices_vbo);
    glBindVertexArray(0);
    glDeleteVertexArrays(NUM_VAO, vao);
    glDeleteProgram(rendering_program);
    glDeleteProgram(outline_program);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


