#version 460

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D u_sceneTexture;
uniform float u_time;
uniform bool u_underwater;
uniform vec3 u_waterColor;
uniform float u_fogDensity;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);   

    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

float fbm(vec2 p) {
    float value = 0.0;
    float amp = 0.5;
    float freq = 1.0;
    for (int i = 0; i < 4; ++i) {
        value += amp * noise(p * freq);
        freq *= 2.0;
        amp *= 0.5;
    }
    return value;
}

float getCausticValue(float x, float y, float z) {
    float w = 8.0;      
    float strength = 4.0;  
    vec2 coord = vec2(x, y) * w + z * 0.5; 
 
    float n = fbm(coord);

    float caustic = pow(n, 3.0) * strength;
    return caustic;
}

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

    float rawCaustic = getCausticValue(TexCoord.x, TexCoord.y, u_time * 0.3);
    float caustic = 0.5 + rawCaustic * 0.5;    
    vec3 causticLight = vec3(caustic * 0.9, caustic, caustic * 0.7);

    float fogFactor = clamp(1.0 - (TexCoord.y * u_fogDensity * 10.0), 0.0, 1.0);
    vec3 mixed = mix(u_waterColor, distorted.rgb * causticLight, fogFactor);

    FragColor = vec4(mixed, 1.0);
}