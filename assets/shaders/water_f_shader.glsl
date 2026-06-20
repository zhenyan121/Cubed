#version 460

layout (location = 0) out vec4 accum;
layout (location = 1) out float reveal;

in vec2 tc;
in vec3 normal;
in vec3 vert_pos;
in vec3 world_pos;
in float roughness;
flat in int tex_layer;
in float v_depth;
layout (binding = 0) uniform sampler2DArray samp;
uniform sampler2D sceneColorTex;   // binding 1
uniform sampler2D sceneDepthTex;   // binding 2

uniform mat4 proj_matrix;
uniform mat4 inv_proj_matrix;
uniform mat4 inv_view_matrix;

uniform float ambientStrength;
uniform vec3 sunlightColor;
uniform vec3 ambientColor;
uniform vec3 sunlightDir;
uniform vec3 cameraPos;
uniform bool shader_on;
uniform float specularStrength;

uniform vec3 skyTop;
uniform vec3 skyBottom;
uniform vec3 sunColor;
uniform float horizonSharpness;
uniform float cloudWhiteMix;
uniform float cloudThresholdLow;
uniform float cloudThresholdHigh;
uniform float time;

uniform vec3 sunDir; // world!

uniform bool underwater;

uniform float refractStrength;

uniform bool enablePerturb;
uniform bool enableDepthFade;
float weight(float z, float a) {
    float intermediate = 0.03 / (1e-5 + pow(z / 200.0, 4.0));
    
    return a * clamp(intermediate, 1e-2, 3e2);
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 5; i++) {
        v += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return v;
}

vec3 computeSkyColor(vec3 dir) {
    vec3 sund = normalize(sunDir);

    float t =
        clamp(
            dir.y * 0.5 + 0.5,
            0.0,
            1.0
        );


    vec3 sky =
        mix(
            skyBottom,
            skyTop,
            pow(t, horizonSharpness)
        );

    // cloud
    if (dir.y > 0.0) {
        vec2 cloud_uv = dir.xz / (dir.y + 0.15) * 0.5 + vec2(time * 0.005, time * 0.002);
        float cloud_density = fbm(cloud_uv * 2.0);
        float safeLow = cloudThresholdLow;
        float safeHigh = max(cloudThresholdHigh, cloudThresholdLow + 0.001);
        cloud_density = smoothstep(safeLow,safeHigh, cloud_density);

        
        float fade = smoothstep(0.0, 0.3, dir.y) * (1.0 - smoothstep(0.85, 1.0, dir.y));
        cloud_density *= fade;

        vec3 cloud_color = mix(skyBottom, vec3(1.0), cloudWhiteMix); 
        sky = mix(sky, cloud_color, cloud_density * 0.6);
    }

    float sunAmount = max(dot(dir, sund), 0.0);
        
    //float glow = pow(sunAmount, 8.0) * 0.15;
        
    float glow = pow(sunAmount, 8.0) * 0.15 + pow(sunAmount, 32.0) * 0.3;

    sky += glow * sunColor;

    return sky;
}

// Reconstruct eye-space coordinates from screen UV and depth buffer value
vec3 reconstructViewPos(vec2 uv, float depth) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = inv_proj_matrix * clip;
    return view.xyz / view.w;
}
// Screen-space ray marching, origin/dir are in eye space
bool traceSSR(vec3 origin, vec3 dir, out vec2 hitUV) {
    const int COARSE_STEPS = 32;
    const float COARSE_STEP = 0.6;   
    const int REFINE_STEPS = 8;
    const float THICKNESS = 0.3;

    vec3 pos = origin;
    vec3 prevPos = origin;
    bool foundCoarse = false;


    for (int i = 0; i < COARSE_STEPS; ++i) {
        prevPos = pos;
        pos += dir * COARSE_STEP;

        vec4 clip = proj_matrix * vec4(pos, 1.0);
        if (clip.w <= 0.0) return false;
        vec3 ndc = clip.xyz / clip.w;
        if (abs(ndc.x) > 1.0 || abs(ndc.y) > 1.0) return false;

        vec2 uv = ndc.xy * 0.5 + 0.5;
        float sceneDepth = texture(sceneDepthTex, uv).r;
        vec3 scenePos = reconstructViewPos(uv, sceneDepth);

        if (pos.z - scenePos.z < 0.0) {
            foundCoarse = true;
            break;   
        }
    }

    if (!foundCoarse) return false;

 
    vec3 lo = prevPos;
    vec3 hi = pos;

    for (int i = 0; i < REFINE_STEPS; ++i) {
        vec3 mid = (lo + hi) * 0.5;

        vec4 clip = proj_matrix * vec4(mid, 1.0);
        vec3 ndc = clip.xyz / clip.w;
        vec2 uv = ndc.xy * 0.5 + 0.5;
        float sceneDepth = texture(sceneDepthTex, uv).r;
        vec3 scenePos = reconstructViewPos(uv, sceneDepth);

        if (mid.z - scenePos.z < 0.0) {
            hi = mid;  
        } else {
            lo = mid; 
        }
    }

    vec4 finalClip = proj_matrix * vec4(hi, 1.0);
    vec3 finalNdc = finalClip.xyz / finalClip.w;
    vec2 finalUV = finalNdc.xy * 0.5 + 0.5;

    float finalDepth = texture(sceneDepthTex, finalUV).r;
    vec3 finalScenePos = reconstructViewPos(finalUV, finalDepth);

    if (abs(hi.z - finalScenePos.z) > THICKNESS) return false;

    hitUV = finalUV;
    return true;
}
void main() {

    vec4 objectColor = texture(samp, vec3(tc, tex_layer));

    if (!shader_on) {
        vec4 color = objectColor;
        float alpha = color.a;
        if (alpha < 1e-4) discard;


        float w = weight(v_depth, alpha);

        accum = vec4(color.rgb * alpha * w, alpha * w);

        reveal = alpha;
        return;
    }
    // Normal perturbation
    vec3 N;
    if (enablePerturb) {
        const float wave_speed_scale = 40.0;
        vec2 waveUV1 = world_pos.xz * 8.0 + vec2(time * 0.05 * wave_speed_scale, time * 0.03 * wave_speed_scale);
        vec2 waveUV2 = world_pos.xz * 13.0 + vec2(-time * 0.04 * wave_speed_scale, time * 0.06 * wave_speed_scale);

        float wave1 = noise(waveUV1);
        float wave2 = noise(waveUV2);

        vec2 normalPerturb = vec2(wave1 - 0.5, wave2 - 0.5) * 0.1;
        N = normalize(normal + vec3(normalPerturb.x, 0.0, normalPerturb.y));
    } else {
        N = normalize(normal);
    }

    vec3 V = normalize(-vert_pos); 

    vec3 reflectColor;
    vec3 refractColor;
    float fresnel;

    vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(sceneDepthTex, 0));

    // water depth
    float sceneDepthRaw = texture(sceneDepthTex, screenUV).r;
    vec3 sceneViewPos = reconstructViewPos(screenUV, sceneDepthRaw);
    float waterDepth = vert_pos.z - sceneViewPos.z;  
    waterDepth = max(waterDepth, 0.0);  

    const float DEPTH_FADE_DISTANCE = 10; 
    float depthFactor = clamp(waterDepth / DEPTH_FADE_DISTANCE, 0.0, 1.0);

    if (underwater) {
        reflectColor = vec3(0.0);
        refractColor = objectColor.rgb; 
        fresnel = 0.0;
    } else {
        vec3 R = reflect(-V, N);
        vec3 origin = vert_pos + N * 0.05;
        vec2 hitUV;

        if (traceSSR(origin, R, hitUV)) {
            reflectColor = texture(sceneColorTex, hitUV).rgb;
        } else {
            vec3 worldR = mat3(inv_view_matrix) * R;
            reflectColor = computeSkyColor(worldR);
        }

        float effectiveRefractStrength = refractStrength * (1.0 - depthFactor * 0.5);
        vec2 refractOffset = N.xz * effectiveRefractStrength;
        vec2 refractUV = clamp(screenUV + refractOffset, vec2(0.001), vec2(0.999));

        refractColor = texture(sceneColorTex, refractUV).rgb;

        fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);
        fresnel = mix(0.02, 1.0, fresnel);
    }
   
    vec3 lightDir = normalize(-sunlightDir);
    vec3 H = normalize(lightDir + V);
    vec3 ambient = ambientStrength * ambientColor;
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = diff * sunlightColor;

    float r = clamp(roughness, 0.0, 1.0);
    float shininess = mix(512.0, 4.0, r);
    float ks = mix(0.8, 0.02, r);

    float spec = 0.0;
    if (diff > 0.0) {
        spec = ks * pow(max(dot(N, H), 0.0), shininess);
    }
    vec3 specular = spec * sunlightColor * specularStrength;

    vec3 tintedRefract;

    if (enableDepthFade) {
        vec3 shallowColor = vec3(0.4, 0.75, 0.7);   
        vec3 deepColor = vec3(0.0, 0.015, 0.045); 

        vec3 waterTint = mix(shallowColor, deepColor, depthFactor);

        tintedRefract = mix(refractColor, refractColor * waterTint, depthFactor);
        tintedRefract = mix(tintedRefract, objectColor.rgb, 0.4);  // Forcefully blend 40% texture color, independent of depth
    } else {
        tintedRefract = mix(refractColor, objectColor.rgb, 0.4);
    }

    //vec3 baseWaterColor = (ambient + diffuse) * objectColor.rgb + specular;
    vec3 baseWaterColor = (ambient + diffuse) * tintedRefract + specular;
  
    vec3 finalColor = mix(baseWaterColor, reflectColor, fresnel);

    float alpha = objectColor.a;
    if (alpha < 1e-4) discard;


    float w = weight(v_depth, alpha);

    accum = vec4(finalColor * alpha * w, alpha * w);

    reveal = alpha;

    return;
}