#version 460

in vec2 tc;
in vec3 normal;
in vec3 vert_pos;
in vec4 FragPosLightSpace;
in float roughness;
flat in int tex_layer;
out vec4 color;
layout (binding = 0) uniform sampler2D shadowMap;
layout (binding = 1) uniform sampler2DArray samp;

uniform float ambientStrength;
uniform vec3 sunlightColor;
uniform vec3 ambientColor;
uniform vec3 sunlightDir;
uniform vec3 cameraPos;
uniform bool shader_on;
uniform int shadowMode;
uniform float specularStrength;
uniform float lightSizeUV; 
uniform float minRadius;
uniform float maxRadius;
const vec2 poissonDisk32[32] = vec2[](
    vec2(-0.975402, -0.071138),
    vec2(-0.920347, -0.411420),
    vec2(-0.883908,  0.217872),
    vec2(-0.815442, -0.879125),
    vec2(-0.775043,  0.543896),
    vec2(-0.698126, -0.227570),
    vec2(-0.682433,  0.801894),
    vec2(-0.563905,  0.021517),
    vec2(-0.443233, -0.975116),
    vec2(-0.412231,  0.361307),
    vec2(-0.264969, -0.418930),
    vec2(-0.241888,  0.997065),
    vec2(-0.094184, -0.929389),
    vec2(-0.019101,  0.680997),
    vec2( 0.143832, -0.141008),
    vec2( 0.199841,  0.786414),

    vec2( 0.344959,  0.293878),
    vec2( 0.443233, -0.475115),
    vec2( 0.537430, -0.473734),
    vec2( 0.589349,  0.569135),
    vec2( 0.674281, -0.178897),
    vec2( 0.791975,  0.190902),
    vec2( 0.815442,  0.879125),
    vec2( 0.896420, -0.613392),
    vec2( 0.945586, -0.768907),
    vec2( 0.974844,  0.756484),
    vec2(-0.814100,  0.914376),
    vec2(-0.382775,  0.276768),
    vec2(-0.915886,  0.457714),
    vec2( 0.537800,  0.912200),
    vec2(-0.620000, -0.650000),
    vec2( 0.120000, -0.780000)
);

const vec2 poissonDisk16[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
);
const vec2 poissonDisk8[8] = vec2[](
    vec2( 0.1440,  0.7659), vec2(-0.5761,  0.4479),
    vec2(-0.3220, -0.6058), vec2( 0.5693, -0.4048),
    vec2(-0.1276,  0.1657), vec2(-0.0649, -0.0165),
    vec2( 0.2773, -0.0305), vec2(-0.1134, -0.2122)
);
uniform int samples;
float random(vec3 seed) {
    return fract(sin(dot(seed, vec3(12.9898,78.233,45.5432))) * 43758.5453);
}

float FindBlocker(vec2 uv,
                  float zReceiver,
                  vec2 texelSize,
                  float bias,
                  float lightSizeUV)
{
    float avgDepth = 0.0;
    int blockers = 0;

    float searchRadius = lightSizeUV * 0.5;

    for(int i = 0; i < samples; i++)
    {    
        vec2 offset;
        if (samples == 32) {
            offset =
            poissonDisk32[i]
            * searchRadius
            * texelSize;
        } else if (samples == 16) {
            offset =
            poissonDisk16[i]
            * searchRadius
            * texelSize;
        } else if (samples == 8) {
            offset =
            poissonDisk8[i]
            * searchRadius
            * texelSize;
        } else {
            offset =
            poissonDisk32[i]
            * searchRadius
            * texelSize;
        }
        float depth =
            texture(shadowMap, uv + offset).r;

        if(depth < zReceiver - bias)
        {
            avgDepth += depth;
            blockers++;
        }
    }

    if(blockers == 0)
        return -1.0;

    return avgDepth / blockers;
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
    float shadow = 0.0;

    float bias =
    clamp(
        0.001 * (1.0 - dot(norm, lightDir)),
        0.0003,
        0.003
    );

    if (shadowMode == 0) {
        vec3 seed = vert_pos * 37.0 + sin(vert_pos * 91.7) * 13.0;
        float angle = random(seed) * 6.2831853;; // 2*PI
        float s = sin(angle), c = cos(angle);
        mat2 rot = mat2(c, -s, s, c);
        //float radius = 0.7;
        float radius = mix(1.0, 4.0, currentDepth);
        
        for (int i = 0; i < samples; ++i) {
            vec2 offset;
            if (samples == 32) {
                offset = rot * poissonDisk32[i] * radius * texelSize;
            } else if (samples == 16) {
                offset = rot * poissonDisk16[i] * radius * texelSize;
            } else if (samples == 8) {
                offset = rot * poissonDisk8[i] * radius * texelSize;
            } else {
               offset = rot * poissonDisk32[i] * radius * texelSize;
            }
            float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
            shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0);
        }
        shadow /= float(samples);
    } else if (shadowMode == 1) {
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                vec2 offset = vec2(x, y) * texelSize;
                float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
                shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0);
            }
        }
        shadow /= 9.0;
    } else if (shadowMode == 2) {
        // pcf off
        float pcfDepth =
        texture(shadowMap, projCoords.xy).r;

        shadow =
        currentDepth - bias > pcfDepth
            ? 1.0
            : 0.0;
    } else if (shadowMode == 3) {
        float avgBlockerDepth =
            FindBlocker(
                projCoords.xy,
                currentDepth,
                texelSize,
                bias,
                lightSizeUV
            );

        if(avgBlockerDepth < 0.0)
        {
            return 0.0;
        }

        vec3 seed = vert_pos * 37.0 + sin(vert_pos * 91.7) * 13.0;
        float angle = random(seed) * 6.2831853;; // 2*PI
        float s = sin(angle), c = cos(angle);
        mat2 rot = mat2(c, -s, s, c);
        /*
        float penumbraRatio = (currentDepth - avgBlockerDepth);
        float radius = clamp(
            penumbraRatio * lightSizeUV,
            minRadius,
            maxRadius
        );
        */
        float radius =
            mix(
                minRadius,
                maxRadius,
                smoothstep(
                    0.0,
                    0.05,
                    currentDepth - avgBlockerDepth
                )
            );
        for (int i = 0; i < samples; ++i) {
            vec2 offset;
            if (samples == 32) {
                offset = rot * poissonDisk32[i] * radius * texelSize;
            } else if (samples == 16) {
                offset = rot * poissonDisk16[i] * radius * texelSize;
            } else if (samples == 8) {
                offset = rot * poissonDisk8[i] * radius * texelSize;
            } else {
               offset = rot * poissonDisk32[i] * radius * texelSize;
            }
            float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
            shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0);
        }
        shadow /= float(samples);

    } else {
        float pcfDepth =
        texture(shadowMap, projCoords.xy).r;

        shadow =
        currentDepth - bias > pcfDepth
            ? 1.0
            : 0.0;
    }

   
    return shadow;
}


void main(void) {
    vec4 objectColor = texture(samp, vec3(tc, tex_layer));

    if (objectColor.a < 0.8) {
        discard;
    }
    if (!shader_on) {
        color = objectColor;
        return;
    }
   

    vec3 lightDir = normalize(-sunlightDir);
    
    vec3 norm = normalize(normal);

    vec3 V =
        normalize(cameraPos - vert_pos);

    vec3 H = 
        normalize(lightDir + V);

    vec3 ambient = ambientStrength * ambientColor;
    
    float diff = max(dot(norm, lightDir), 0.0);
    
    vec3 diffuse = diff * sunlightColor;
    
    float r =
        clamp(roughness, 0.0, 1.0);

    float shininess =
        mix(
            512.0,
            4.0,
            r
        );
    float ks =
        mix(
            0.8,
            0.02,
            r
        );

    float spec = 0.0;

    if(diff > 0.0)
    {
        spec =
            ks *
            pow(
                max(dot(norm,H),0.0),
                shininess
            );
    }



    
    vec3 specular = spec * sunlightColor * specularStrength;

    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);  
  
    color = vec4((ambient + (1.0 - shadow) * (diffuse)) * objectColor.rgb + (1.0-shadow) * specular, objectColor.a);

    //color = varyingColor;
}
