#version 460

layout (location = 0) out vec4 accum;
layout (location = 1) out float reveal;

in vec2 tc;
flat in int tex_layer;
in float v_depth;
layout (binding = 0) uniform sampler2DArray samp;

float weight(float z, float a) {
    float intermediate = 0.03 / (1e-5 + pow(z / 200.0, 4.0));
    
    return a * clamp(intermediate, 1e-2, 3e2);
}

void main() {
    vec4 color = texture(samp, vec3(tc, tex_layer));
    float alpha = color.a;
    if (alpha < 1e-4) discard;


    float w = weight(v_depth, alpha);

    accum = vec4(color.rgb * alpha * w, alpha * w);

    reveal = alpha;
}