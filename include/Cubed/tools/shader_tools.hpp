#pragma once

#include <glad/glad.h>
#include <SOIL2.h>
#include <string>


void print_shader_log(GLuint shader);
void print_program_info(int prog);
bool check_opengl_error();
std::string read_shader_source(const char* file_path);
GLuint load_texture(const char* tex_image_path);