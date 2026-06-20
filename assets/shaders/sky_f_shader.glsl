#version 460

in vec3 dir;

out vec4 frag_color;


uniform vec3 skyTop;
uniform vec3 skyBottom;

uniform vec3 sunDir;
uniform vec3 sunColor;

uniform float horizonSharpness;
uniform float cloudWhiteMix;

uniform float cloudThresholdLow; 
uniform float cloudThresholdHigh;

uniform float time;


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

void main(void) {
    
    vec3 sky = computeSkyColor(dir);

    frag_color = vec4(sky, 1.0);
    //frag_color = vec4(vec3(sunAmount), 1.0);
    //frag_color = vec4(t,0,0,1);
    
}