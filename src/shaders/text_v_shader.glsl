#version 460

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float layer;

out vec2 tc;
flat out int tex_layer;
uniform mat4 projection;

void main(void) {
    gl_Position = projection * vec4(pos, 0.0, 1.0);
    tc = texCoord;
    tex_layer = int(layer);
}