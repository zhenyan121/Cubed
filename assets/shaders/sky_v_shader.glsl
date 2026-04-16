#version 460

layout (location = 0) in vec3 vertices_pos;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

void main(void) {
    gl_Position = proj_matrix * mv_matrix * vec4(vertices_pos, 1.0);
}