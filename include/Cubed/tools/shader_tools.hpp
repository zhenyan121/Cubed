#pragma once

#include <glad/glad.h>
#include <SOIL2.h>
#include <string>

namespace Tools {
    GLuint create_shader_program(const std::string& v_shader_path, const std::string& f_shader_path);
    void print_shader_log(GLuint shader);
    void print_program_info(int prog);
    bool check_opengl_error();
    std::string read_shader_source(const std::string& file_path);
    void delete_image_data(unsigned char* data);
    unsigned char* load_image_data(const std::string& tex_image_path);

}
