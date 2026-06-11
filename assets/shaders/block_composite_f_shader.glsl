#version 460

uniform sampler2D u_accumTex;
uniform sampler2D u_revealTex;
in vec2 TexCoord;
out vec4 FragColor;

void main() {
    vec4 a = texture(u_accumTex, TexCoord);
    float r = texture(u_revealTex, TexCoord).r;

    if (a.a < 1e-4) discard;

    vec3 color = a.rgb / max(a.a, 1e-5);
    float transmittance = r;
    float opacity = 1.0 - transmittance;

    FragColor = vec4(color * opacity, opacity);
}
