#version 460

in vec2 tc;
in vec3 normal;
in vec3 vert_pos;
in vec4 FragPosLightSpace;
flat in int tex_layer;
out vec4 color;
layout (binding = 0) uniform sampler2D shadowMap;
layout (binding = 1) uniform sampler2DArray samp;

uniform float ambientStrength;
uniform vec3 sunlightColor;
uniform vec3 sunlightDir;

const vec2 poissonDisk[8] = vec2[](
    vec2( 0.1440,  0.7659), vec2(-0.5761,  0.4479),
    vec2(-0.3220, -0.6058), vec2( 0.5693, -0.4048),
    vec2(-0.1276,  0.1657), vec2(-0.0649, -0.0165),
    vec2( 0.2773, -0.0305), vec2(-0.1134, -0.2122)
);

float random(vec3 seed) {
    return fract(sin(dot(seed, vec3(12.9898,78.233,45.5432))) * 43758.5453);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 norm, vec3 lightDir)
{

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float bias = 0.0002;
    bias += max(0.005 * texelSize.x * (1.0 - dot(norm, lightDir)), 0.0);

    float angle = random(gl_FragCoord.xyy) * 6.2831853; // 2*PI
    float s = sin(angle), c = cos(angle);
    mat2 rot = mat2(c, -s, s, c);

    float radius = 1.5;

    float shadow = 0.0;
    const int samples = 8;
    for (int i = 0; i < samples; ++i) {
        vec2 offset = rot * poissonDisk[i] * radius * texelSize;
        float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
        shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0);
    }
    shadow /= float(samples);

    return shadow;
}


void main(void) {
    vec4 objectColor = texture(samp, vec3(tc, tex_layer));
    
    if (objectColor.a < 0.8) {
        discard;
    }

    vec3 lightDir = normalize(-sunlightDir);
    
    vec3 ambient = ambientStrength * sunlightColor;
    
    vec3 norm = normalize(normal);
    
    float diff = max(dot(norm, lightDir), 0.0);
    
    vec3 diffuse = diff * sunlightColor;

    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);  
  
    color = vec4((ambient + (1.0 - shadow) * (diffuse)) * objectColor.rgb, objectColor.a);

    //color = varyingColor;
}
