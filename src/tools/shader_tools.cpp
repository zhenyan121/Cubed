#include <fstream>

#include <Cubed/config.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/shader_tools.hpp>
#include <Cubed/tools/log.hpp>


namespace Shader {
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

    void delete_image_data(unsigned char* data) {
        SOIL_free_image_data(data);
    }

    unsigned char* load_image_data(const std::string& tex_image_path) {
        unsigned char* data;
        int width, height, channels; 
        data = SOIL_load_image(tex_image_path.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
        std::string error_info = "Could not load texture " + tex_image_path;
        CUBED_ASSERT_MSG(data, error_info.c_str());

        return data;
    }

}
