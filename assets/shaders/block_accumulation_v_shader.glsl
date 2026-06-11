#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float layer;
out vec2 tc;
flat out int tex_layer;
out float v_depth;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;



void main(void) {
    vec4 view_pos = mv_matrix * vec4(pos, 1.0);
    gl_Position = proj_matrix * view_pos;
    tc = texCoord;
    tex_layer = int(layer);
    v_depth = -view_pos.z;
}
