#version 460

in vec2 tc;
flat in int tex_layer;

layout (binding = 0) uniform sampler2DArray text;
uniform vec3 textColor;

out vec4 color;

void main(void) {
    vec4 smapled = vec4(1.0, 1.0, 1.0, texture(text, vec3(tc, tex_layer)).r);
    color = vec4(textColor, 1.0) * smapled;
}