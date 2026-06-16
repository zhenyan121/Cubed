#version 460
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float layer;
uniform mat4 lightSpaceMatrix;
out vec2 tc;
flat out int tex_layer;
void main() {
    tc = texCoord;
    tex_layer = int(layer);
    gl_Position = lightSpaceMatrix  * vec4(pos, 1.0);
}