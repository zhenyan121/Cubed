#version 460

layout (location = 0) in vec3 vertices_pos;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

out vec3 dir;

void main(void) {
    // Our skybox mesh uses [0,1] coordinates instead of the usual [-1,1].
    // Shift to the cube center before normalizing to obtain the correct
    // view direction for atmospheric calculations.
    dir = normalize(vertices_pos - vec3(0.5));
    gl_Position = proj_matrix * mv_matrix * vec4(vertices_pos, 1.0);
}