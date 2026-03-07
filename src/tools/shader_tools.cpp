#include <fstream>

#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/shader_tools.hpp>
#include <Cubed/tools/log.hpp>



void print_shader_log(GLuint shader) {
    int len = 0;
    int ch_written = 0;
    char *log;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        log = (char*)malloc(len);
        glGetShaderInfoLog(shader, len, &ch_written, log);
        LOG::info("Shader Info Log: {}", log);
        free(log);
    }

} 


void print_program_info(int prog) {
    int len = 0;
    int ch_written = 0;
    char *log;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        log = (char*)malloc(len);
        glGetProgramInfoLog(prog, len, &ch_written, log);
        LOG::info("Program Info Log: {}", log);
        free(log);
    }
}

bool check_opengl_error() {
    bool found_error = false;
    int gl_err = glGetError();
    while (gl_err != GL_NO_ERROR) {
        LOG::error("glEorr: {}", gl_err);
        found_error = true;
        gl_err = glGetError();
    }

    return found_error;
}


std::string read_shader_source(const char* file_path) {
    std::string content;
    std::ifstream file_stream(file_path, std::ios::in);

    if (!file_stream.is_open()) {
        LOG::error("{} not exist", file_path);
    }

    std::string line = "";
    while (!file_stream.eof()) {
        
        getline(file_stream, line);
        content.append(line + "\n");
    }
    file_stream.close();
    return content;
}

GLuint load_texture(const std::string& tex_image_path) {
    return load_texture(tex_image_path.c_str());
}

GLuint load_texture(const char* tex_image_path) {
    GLuint texture_id;
    texture_id = SOIL_load_OGL_texture(tex_image_path, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
    std::string error_info = std::string("Could not load texture") + tex_image_path;
    CUBED_ASSERT_MSG(texture_id, error_info.c_str());
    return texture_id;
}