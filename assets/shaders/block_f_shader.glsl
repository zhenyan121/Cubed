#version 460

in vec2 tc;
in vec3 normal;
in vec3 vert_pos;
flat in int tex_layer;
out vec4 color;

layout (binding = 0) uniform sampler2DArray samp;

uniform float ambientStrength;
uniform vec3 sunlightColor;
uniform vec3 sunlightDir;

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
    
    color = vec4((ambient + diffuse) * objectColor.rgb, objectColor.a);

    //color = varyingColor;
}
