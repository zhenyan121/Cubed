#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Cubed/camera.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/shader_tools.hpp>

constexpr int numVAOs = 1;
constexpr int numVBOs = 3;
constexpr int WORLD_SIZE_X = 32;
constexpr int WORLD_SIZE_Z = 32;
bool blockPresent[WORLD_SIZE_X][WORLD_SIZE_Z] = {false};
GLuint renderingProgram;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];
GLuint mvLoc, projLoc;
int width ,height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat;
float inc = 0.01f;
float tf = 0.0f;
double lastTime = 0.0f;
double deltaTime = 0.0f;
glm::mat4 tMat, rMat;
std::vector<GLuint> grass_block_texture(6);
Player player;
Camera camera;
void setupVertices(void) {
    float verticesPos[108] = {
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

    
    glGenVertexArrays(numVAOs, vao);
    glBindVertexArray(vao[0]);
    glGenBuffers(numVBOs, vbo);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesPos), verticesPos, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}




GLuint createShaderProgram() {
    std::string vShaderStr = readShaderSource("shaders/vShader.glsl");
    std::string fShaderStr = readShaderSource("shaders/fShader.glsl");
    const char *vshaderSource = vShaderStr.c_str();
    const char *fshaderSource = fShaderStr.c_str();

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

    GLint vc, fc;
    glShaderSource(vShader, 1, &vshaderSource, NULL);
    glShaderSource(fShader, 1, &fshaderSource, NULL);
    glCompileShader(vShader);
    checkOpenGLError();
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &vc);
    if (vc != 1) {
        LOG::error("vertex compilation failed");
        printShaderLog(vShader);
    }
    glCompileShader(fShader);         
    checkOpenGLError();
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &fc);
    if (fc != 1) {
        LOG::error("vertex compilation failed");
        printShaderLog(fShader);
    }
    GLuint vfProgram = glCreateProgram();
    glAttachShader(vfProgram, vShader);
    glAttachShader(vfProgram, fShader);
    glLinkProgram(vfProgram);

    GLint linked;
    checkOpenGLError();
    glGetProgramiv(vfProgram, GL_LINK_STATUS, &linked);
    if (linked != 1) {
        LOG::error("linking failed");
        printProgramInfo(vfProgram);
    }

    return vfProgram;
}


void init(GLFWwindow* window) {
    renderingProgram = createShaderProgram();

    mvLoc = glGetUniformLocation(renderingProgram, "mv_matrix");
    projLoc = glGetUniformLocation(renderingProgram, "proj_matrix");

    camera.cameraInit(&player);
    glfwGetFramebufferSize(window, &width, &height);
    aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    pMat = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f); 
    
    grass_block_texture[0] = loadTexture("assets/texture/block/grass_block/front.png");
    grass_block_texture[1] = loadTexture("assets/texture/block/grass_block/right.png");
    grass_block_texture[2] = loadTexture("assets/texture/block/grass_block/back.png");
    grass_block_texture[3] = loadTexture("assets/texture/block/grass_block/left.png");
    grass_block_texture[4] = loadTexture("assets/texture/block/grass_block/top.png");
    grass_block_texture[5] = loadTexture("assets/texture/block/grass_block/base.png");

    for (int i = 0; i < 6; i++) {
        glBindTexture(GL_TEXTURE_2D, grass_block_texture[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);    
    
    setupVertices();

}

void window_reshape_callback(GLFWwindow* window, int newWidth, int newHeight) {
    glfwGetFramebufferSize(window, &width, &height);
    aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    pMat = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
}




void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    camera.updateCursorPositionCamera(xpos, ypos);
}



void display(GLFWwindow* window, double currentTime) {
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    player.update(deltaTime);
    camera.updateMoveCamera();

    for (int i = 0; i < WORLD_SIZE_X; i++) {
        for (int j = 0; j < WORLD_SIZE_Z; j++) {
            blockPresent[i][j] = true;
        }
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(renderingProgram);
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

            mMat = glm::translate(glm::mat4(1.0f), glm::vec3((float)wx, 0.0f, (float)wz));
            vMat = camera.getCameraLookAt();
            mvMat = vMat * mMat;
            glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
            glUniformMatrix4fv(projLoc, 1 ,GL_FALSE, glm::value_ptr(pMat));

            bool drawFace[6] = {true, true, true, true, true, true};

            if (z < WORLD_SIZE_Z - 1&& blockPresent[x][z + 1]) {
                drawFace[0] = false;
            }
            if (x < WORLD_SIZE_X - 1 && blockPresent[x + 1][z]) {
                drawFace[1] = false;
            }
            if (z > 0 && blockPresent[x][z - 1]) {
                drawFace[2] = false;
            }
            if (x > 0 && blockPresent[x - 1][z]) {
                drawFace[3] = false;
            }


            for (int face = 0; face < 6; face++) {
                if (!drawFace[face]) {
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

    player.updatePlayerMoveState(key, action);
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


