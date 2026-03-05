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
GLuint pyrTexture;
float cubLocX, cubLocY, cubLocZ;
float pyLocX, pyLocY, pyLocZ;
GLuint mvLoc, projLoc, tfLoc;
int width ,height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat;
float inc = 0.01f;
float tf = 0.0f;
glm::mat4 tMat, rMat, sMat;
std::vector<GLuint> grass_block_texture(6);

void setupVertices(void) {
    float verticesPos[108] = {
        // ===== 前面 (z = +1) =====
        -0.5f, -0.5f,  0.5f,  // 左下
        -0.5f,  0.5f,  0.5f,  // 左上
        0.5f,  0.5f,  0.5f,  // 右上
        0.5f,  0.5f,  0.5f,  // 右上
        0.5f, -0.5f,  0.5f,  // 右下
        -0.5f, -0.5f,  0.5f,  // 左下
        // ===== 右面 (x = +1) =====
        0.5f, -0.5f,  0.5f,  // 前下
        0.5f, -0.5f, -0.5f,  // 后下
        0.5f,  0.5f, -0.5f,  // 后上
        0.5f,  0.5f, -0.5f,  // 后上
        0.5f,  0.5f,  0.5f,  // 前上
        0.5f, -0.5f,  0.5f,  // 前下
        // ===== 后面 (z = -1) =====
        -0.5f, -0.5f, -0.5f,  // 左下
        0.5f, -0.5f, -0.5f,  // 右下
        0.5f,  0.5f, -0.5f,  // 右上
        0.5f,  0.5f, -0.5f,  // 右上
        -0.5f,  0.5f, -0.5f,  // 左上
        -0.5f, -0.5f, -0.5f,  // 左下
        // ===== 左面 (x = -1) =====
        -0.5f, -0.5f, -0.5f,  // 后下
        -0.5f, -0.5f,  0.5f,  // 前下
        -0.5f,  0.5f,  0.5f,  // 前上
        -0.5f,  0.5f,  0.5f,  // 前上
        -0.5f,  0.5f, -0.5f,  // 后上
        -0.5f, -0.5f, -0.5f,  // 后下
        // ===== 上面 (y = +1) =====
        -0.5f,  0.5f, -0.5f,  // 后左
        0.5f,  0.5f, -0.5f,  // 后右
        0.5f,  0.5f,  0.5f,  // 前右
        0.5f,  0.5f,  0.5f,  // 前右
        -0.5f,  0.5f,  0.5f,  // 前左
        -0.5f,  0.5f, -0.5f,  // 后左
        // ===== 下面 (y = -1) =====
        -0.5f, -0.5f,  0.5f,  // 前左
        0.5f, -0.5f,  0.5f,  // 前右
        0.5f, -0.5f, -0.5f,  // 后右
        0.5f, -0.5f, -0.5f,  // 后右
        -0.5f, -0.5f, -0.5f,  // 后左
        -0.5f, -0.5f,  0.5f   // 前左
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
    
    cubLocX = 0.0f;
    cubLocY = -2.0f;
    cubLocZ = 0.0f;
    pyLocX = 0.0f;
    pyLocY = -2.0f;
    pyLocZ = -20.0f;
    cameraInit();
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
    updateCursorPositionCamera(xpos, ypos);
}



void display(GLFWwindow* window, double currentTime) {

    updateMoveCamera();

    for (int i = 0; i < WORLD_SIZE_X; i++) {
        for (int j = 0; j < WORLD_SIZE_Z; j++) {
            blockPresent[i][j] = true;
        }
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(renderingProgram);
    
    

    mvLoc = glGetUniformLocation(renderingProgram, "mv_matrix");
    projLoc = glGetUniformLocation(renderingProgram, "proj_matrix");
    
    /*
    cameraX += inc;
    if (cameraX > 1.0f)  {
        inc = -inc;
    }
    if (cameraX < -1.0f) {
        inc = -inc;
    }
    */

    
    glBindVertexArray(vao[0]);
    
    //sMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
    

    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    for (int x = -16; x < 16; x++) {
        for (int z = -16; z < 16; z++) {

            int ix = x + 16;
            int iz = z + 16;

            mMat = glm::translate(glm::mat4(1.0f), glm::vec3((float)x, 0.0f, (float)z));
            vMat = getCameraLookAt();
            mvMat = vMat * mMat;
            glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
            glUniformMatrix4fv(projLoc, 1 ,GL_FALSE, glm::value_ptr(pMat));

            bool drawFace[6] = {true, true, true, true, true, true};

            if (z < 15 && blockPresent[ix][iz + 1]) {
                drawFace[0] = false;
            }
            if (x < 15 && blockPresent[ix + 1][iz]) {
                drawFace[1] = false;
            }
            if (z > -16 && blockPresent[ix][iz + 1]) {
                drawFace[2] = false;
            }
            if (x > -16 && blockPresent[ix - 1][iz]) {
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

    updateCameraKey(key, action);
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


