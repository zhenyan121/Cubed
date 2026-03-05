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


void setupVertices(void) {
    float verticesPos[108] = {
        // ===== 后面 (z = -1) =====
        -1.0f, -1.0f, -1.0f,  // 左下
        1.0f, -1.0f, -1.0f,  // 右下
        1.0f,  1.0f, -1.0f,  // 右上
        1.0f,  1.0f, -1.0f,  // 右上
        -1.0f,  1.0f, -1.0f,  // 左上
        -1.0f, -1.0f, -1.0f,  // 左下
        // ===== 前面 (z = +1) =====
        -1.0f, -1.0f,  1.0f,  // 左下
        -1.0f,  1.0f,  1.0f,  // 左上
        1.0f,  1.0f,  1.0f,  // 右上
        1.0f,  1.0f,  1.0f,  // 右上
        1.0f, -1.0f,  1.0f,  // 右下
        -1.0f, -1.0f,  1.0f,  // 左下
        // ===== 左面 (x = -1) =====
        -1.0f, -1.0f, -1.0f,  // 后下
        -1.0f, -1.0f,  1.0f,  // 前下
        -1.0f,  1.0f,  1.0f,  // 前上
        -1.0f,  1.0f,  1.0f,  // 前上
        -1.0f,  1.0f, -1.0f,  // 后上
        -1.0f, -1.0f, -1.0f,  // 后下
        // ===== 右面 (x = +1) =====
        1.0f, -1.0f,  1.0f,  // 前下
        1.0f, -1.0f, -1.0f,  // 后下
        1.0f,  1.0f, -1.0f,  // 后上
        1.0f,  1.0f, -1.0f,  // 后上
        1.0f,  1.0f,  1.0f,  // 前上
        1.0f, -1.0f,  1.0f,  // 前下
        // ===== 上面 (y = +1) =====
        -1.0f,  1.0f, -1.0f,  // 后左
        1.0f,  1.0f, -1.0f,  // 后右
        1.0f,  1.0f,  1.0f,  // 前右
        1.0f,  1.0f,  1.0f,  // 前右
        -1.0f,  1.0f,  1.0f,  // 前左
        -1.0f,  1.0f, -1.0f,  // 后左
        // ===== 下面 (y = -1) =====
        -1.0f, -1.0f,  1.0f,  // 前左
        1.0f, -1.0f,  1.0f,  // 前右
        1.0f, -1.0f, -1.0f,  // 后右
        1.0f, -1.0f, -1.0f,  // 后右
        -1.0f, -1.0f, -1.0f,  // 后左
        -1.0f, -1.0f,  1.0f   // 前左
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
    
    pyrTexture = loadTexture("assets/texture/block/grass_block/top.png");
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
    mMat = glm::translate(glm::mat4(1.0f), glm::vec3(pyLocX, pyLocY, pyLocZ));
    vMat = getCameraLookAt();
    mvMat = vMat * mMat;
    glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
    glUniformMatrix4fv(projLoc, 1 ,GL_FALSE, glm::value_ptr(pMat));

    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pyrTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    //glPointSize(30.0f);
    
    glDrawArrays(GL_TRIANGLES, 0, 36);
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


