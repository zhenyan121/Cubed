#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float layer;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in float Roughness;

out vec2 tc;
out vec3 normal;
out vec3 vert_pos;
out vec3 world_pos;
flat out int tex_layer;
out float roughness;
out float v_depth;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform mat4 norm_matrix;


void main(void) {
    vec4 view_pos = mv_matrix * vec4(pos, 1.0);
    world_pos = pos;
    vert_pos = view_pos.xyz;
    normal = mat3(norm_matrix) * aNormal;
    roughness = Roughness;
    tc = texCoord;
    tex_layer = int(layer);
    v_depth = -view_pos.z;
    gl_Position = proj_matrix * view_pos;
}
