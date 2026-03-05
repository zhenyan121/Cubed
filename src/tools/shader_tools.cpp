#include <Cubed/tools/shader_tools.hpp>
#include <Cubed/tools/log.hpp>



void printShaderLog(GLuint shader) {
    int len = 0;
    int chWritten = 0;
    char *log;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        log = (char*)malloc(len);
        glGetShaderInfoLog(shader, len, &chWritten, log);
        LOG::info("Shader Info Log: {}", log);
        free(log);
    }

} 


void printProgramInfo(int prog) {
    int len = 0;
    int chWritten = 0;
    char *log;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        log = (char*)malloc(len);
        glGetProgramInfoLog(prog, len, &chWritten, log);
        LOG::info("Program Info Log: {}", log);
        free(log);
    }
}

bool checkOpenGLError() {
    bool foundError = false;
    int glErr = glGetError();
    while (glErr != GL_NO_ERROR) {
        LOG::error("glEorr: {}", glErr);
        foundError = true;
        glErr = glGetError();
    }

    return foundError;
}


std::string readShaderSource(const char* filePath) {
    std::string content;
    std::ifstream fileStream(filePath, std::ios::in);

    if (!fileStream.is_open()) {
        LOG::error("file not exist");
    }

    std::string line = "";
    while (!fileStream.eof()) {
        
        getline(fileStream, line);
        content.append(line + "\n");
    }
    fileStream.close();
    return content;
}

GLuint loadTexture(const char* texImagePath) {
    GLuint textureID;
    textureID = SOIL_load_OGL_texture(texImagePath, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
    if (textureID == 0) {
        LOG::error("could not find texture file");
    }
    return textureID;
}