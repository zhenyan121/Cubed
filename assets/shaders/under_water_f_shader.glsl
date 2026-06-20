#version 460

in vec2 TexCoord;
in vec3 rayDir;

out vec4 FragColor;

layout (binding = 0) uniform sampler2D u_sceneTexture;
layout (binding = 1) uniform sampler2D u_depthTexture;
layout (binding = 2) uniform sampler2D u_shadowMap;
uniform mat4 u_lightSpaceMatrix;
uniform float u_time;
uniform bool u_underwater;
uniform vec3 u_waterColor;
uniform float u_fogDensity;
uniform mat4 InverseViewProjection; 
uniform vec3 cameraPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform float waterDensity;

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

float getShadow(vec3 worldPos) {
    vec4 posLightSpace = u_lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0) return 1.0; 
    
    float closestDepth = texture(u_shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    return currentDepth - 0.005 > closestDepth ? 0.0 : 1.0; 
}

void main() {
    vec4 original = texture(u_sceneTexture, TexCoord);
    
    if (!u_underwater) {
        FragColor = original;
        return;
    }

    float rawDepth = texture(u_depthTexture, TexCoord).r;
    vec3 ndc = vec3(TexCoord * 2.0 - 1.0, rawDepth * 2.0 - 1.0);
    vec4 worldPosH = InverseViewProjection * vec4(ndc, 1.0);
    vec3 worldPos = worldPosH.xyz / worldPosH.w;
    float sceneDistance = distance(cameraPos, worldPos);
    
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
    // Volume Scattering
    vec3 rayOrigin = cameraPos;
    vec3 rayDirection = normalize(rayDir);

    float maxDist = 10.0;        // Maximum visible distance
    float stepSize = 0.4;     
    float phasePower = 8.0;      // Beam Sharpness

    vec3 scattering = vec3(0.0);
    float transmittance = 1.0;
    
    float cosTheta = dot(rayDirection, sunDir);
    float phase = pow(max(cosTheta, 0.0), phasePower);

    for (float t = 0.0; t < maxDist; t += stepSize) {

        if (t > sceneDistance) break;
        vec3 pos = rayOrigin + rayDirection * t;

        float shadow = getShadow(pos);

        float density = waterDensity; 


        vec3 absorption = vec3(0.3, 0.08, 0.02); // Absorption coefficient per meter
        vec3 sunLightAtPos = sunColor * exp(-absorption * t);

        scattering += density * phase * sunLightAtPos * transmittance * stepSize * shadow;

        float extinction = waterDensity * 1.5; // Extinction coefficient, tunable
        transmittance *= exp(-extinction * stepSize);
        if (transmittance < 0.01) break;
    }

    FragColor.rgb = mixed + scattering;
    FragColor.a = 1.0;
}