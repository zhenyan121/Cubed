#version 460

in vec2 texCoord;
out vec4 color;

layout (binding = 0) uniform sampler2D text;
uniform vec3 textColor;

void main(void) {
    vec4 smapled = vec4(1.0, 1.0, 1.0, texture(text, texCoord).r);
    color = vec4(textColor, 1.0) * smapled;
}