#version 460

in vec2 tc;
flat in int tex_layer;
layout (binding = 1) uniform sampler2DArray samp;

void main() {
    vec4 texColor = texture(samp, vec3(tc, tex_layer));
    if (texColor.a < 0.8) 
        discard;
    //gl_FragDepth = gl_FragCoord.z; 
}