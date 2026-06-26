#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

out vec2 tc;

void main() {
    vec4 viewPos = mv_matrix * vec4(pos, 1.0);
    tc = texCoord;
    gl_Position = proj_matrix * viewPos;
}