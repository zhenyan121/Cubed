#version 460

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D u_sceneTexture;
uniform float u_time;
uniform bool u_underwater;
uniform vec3 u_waterColor;
uniform float u_fogDensity;

void main() {
    vec4 original = texture(u_sceneTexture, TexCoord);

    if (!u_underwater) {
        FragColor = original;
        return;
    }

    vec2 distoredUV = TexCoord;
    float strength = 0.003;
    distoredUV.x += sin(TexCoord.y * 15.0 + u_time * 5.0) * strength;
    distoredUV.y += cos(TexCoord.x * 15.0 + u_time * 4.3) * strength;
    distoredUV = clamp(distoredUV, 0.001, 0.999);
    vec4 distorted = texture(u_sceneTexture, distoredUV);

    float caustic = 0.9 + 0.1 * sin(TexCoord.x * 20.0 + u_time) * cos(TexCoord.y * 20.0 + u_time * 1.2);
    vec3 causticLight = vec3(caustic, caustic * 0.95, caustic * 0.9);
    //vec3 causticLight = vec3(1.0);
    float fogFactor = clamp(1.0 - (TexCoord.y * u_fogDensity * 10.0), 0.0, 1.0);
    vec3 mixed = mix(u_waterColor, distorted.rgb * causticLight, fogFactor);

    FragColor = vec4(mixed, 1.0);
}