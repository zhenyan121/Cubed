#version 460

in vec3 dir;

out vec4 frag_color;


uniform vec3 skyTop;
uniform vec3 skyBottom;

uniform vec3 sunDir;
uniform vec3 sunColor;

uniform float horizonSharpness;

void main(void) {
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

    float sunAmount = max(dot(dir, sund), 0.0);
        
    //float glow = pow(sunAmount, 8.0) * 0.15;
        
    float glow = pow(sunAmount, 8.0) * 0.15 + pow(sunAmount, 32.0) * 0.3;

    sky += glow * sunColor;

    frag_color = vec4(sky, 1.0);
    //frag_color = vec4(vec3(sunAmount), 1.0);
    //frag_color = vec4(t,0,0,1);
    
}