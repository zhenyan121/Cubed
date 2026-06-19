#version 460

layout (location = 0) out vec4 accum;
layout (location = 1) out float reveal;

in vec2 tc;
in vec3 normal;
in vec3 vert_pos;
in float roughness;
flat in int tex_layer;
in float v_depth;
layout (binding = 0) uniform sampler2DArray samp;

uniform float ambientStrength;
uniform vec3 sunlightColor;
uniform vec3 ambientColor;
uniform vec3 sunlightDir;
uniform vec3 cameraPos;
uniform bool shader_on;
uniform float specularStrength;

float weight(float z, float a) {
    float intermediate = 0.03 / (1e-5 + pow(z / 200.0, 4.0));
    
    return a * clamp(intermediate, 1e-2, 3e2);
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
  
    vec4 color = vec4((ambient + diffuse) * objectColor.rgb + specular, objectColor.a);

    float alpha = color.a;
    if (alpha < 1e-4) discard;


    float w = weight(v_depth, alpha);

    accum = vec4(color.rgb * alpha * w, alpha * w);

    reveal = alpha;
}