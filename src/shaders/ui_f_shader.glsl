#version 460

in vec2 tc;
flat in int tex_layer;

out vec4 color;

layout (binding = 0) uniform sampler2DArray samp;

void main(void) {
    color = texture(samp, vec3(tc, tex_layer));
}