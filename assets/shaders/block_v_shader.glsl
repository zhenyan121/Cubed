#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float layer;
out vec2 tc;
flat out int tex_layer;

mat4 buildRotateX(float rad);
mat4 buildRotateY(float rad);
mat4 buildRotateZ(float rad);
mat4 buildTranslate(float x, float y, float z);

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;



void main(void) {
    gl_Position = proj_matrix * mv_matrix * vec4(pos, 1.0);
    tc = texCoord;
    tex_layer = int(layer);
}

mat4 buildTranslate(float x, float y, float z) {
    mat4 trans = mat4(
        1.0, 0.0, 0.0, 0.0, 
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        x, y, z, 1.0
    );
    return trans;
}

mat4 buildRotateX(float rad) {
    mat4 xrot = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, cos(rad), -sin(rad), 0.0,
        0.0, sin(rad), cos(rad), 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    return xrot;
}

mat4 buildRotateY(float rad) {
    mat4 yrot = mat4(
        cos(rad), 0.0, sin(rad), 0.0,
        0.0, 1.0, 0.0, 0.0,
        -sin(rad), 0.0, cos(rad), 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    return yrot;
}

mat4 buildRotateZ(float rad) {
    mat4 zrot = mat4(
        cos(rad), -sin(rad), 0.0, 0.0,
        sin(rad), cos(rad), 0.0, 0.0,
        0.0, 0.0,1.0 , 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    return zrot;
}