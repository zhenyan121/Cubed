#version 460

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 texCoord;

uniform mat4 InverseViewProjection; 
uniform vec3 cameraPos;

out vec2 TexCoord;
out vec3 rayDir;
void main() {
    TexCoord = texCoord;
    vec4 clipPos = vec4(pos, 1.0, 1.0);
    vec4 worldPosH = InverseViewProjection * clipPos;
    vec3 worldPos = worldPosH.xyz / worldPosH.w;
    vec3 RayDir = worldPos - cameraPos;
    rayDir = normalize(RayDir);
    gl_Position = clipPos;
    
}