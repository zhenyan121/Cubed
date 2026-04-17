#pragma once
constexpr int WORLD_SIZE_Y = 256;
constexpr int MAX_BLOCK_NUM = 5;
constexpr int MAX_UI_NUM = 1;

constexpr int CHUCK_SIZE = 16;
constexpr int DISTANCE = 16;
constexpr int MAX_BLOCK_STATUS = 1;
constexpr int MAX_CHARACTER = 128;
constexpr float NORMAL_FOV = 70.0f;

constexpr int MAX_BIOME_SUM = 4;
constexpr float BIOME_NOISE_FREQUENCY = 0.005f;

constexpr float VERTICES_POS[6][6][3] = {
        // ===== front (z = +1) =====
        0.0f, 0.0f, 1.0f,  // bottom left
        0.0f, 1.0f, 1.0f,  // top left
        1.0f, 1.0f, 1.0f,  // top right
        1.0f, 1.0f, 1.0f,  // top right
        1.0f, 0.0f, 1.0f,  // bottom right
        0.0f, 0.0f, 1.0f,  // bottom left
        // ===== right (x = +1) =====
        1.0f, 0.0f, 1.0f,  // bottom front
        1.0f, 0.0f, 0.0f,  // bottom back
        1.0f, 1.0f, 0.0f,  // top back
        1.0f, 1.0f, 0.0f,  // top back
        1.0f, 1.0f, 1.0f,  // top front
        1.0f, 0.0f, 1.0f,  // bottom front
        // ===== back (z = -1) =====
        0.0f, 0.0f, 0.0f,  // bottom left
        1.0f, 0.0f, 0.0f,  // bottom right
        1.0f, 1.0f, 0.0f,  // top right
        1.0f, 1.0f, 0.0f,  // top right
        0.0f, 1.0f, 0.0f,  // top left
        0.0f, 0.0f, 0.0f,  // bottom left
        // ===== left (x = -1) =====
        0.0f, 0.0f, 0.0f,  // bottom back
        0.0f, 0.0f, 1.0f,  // bottom front
        0.0f, 1.0f, 1.0f,  // top front
        0.0f, 1.0f, 1.0f,  // top front
        0.0f, 1.0f, 0.0f,  // top back
        0.0f, 0.0f, 0.0f,  // bottom back
        // ===== top (y = +1) =====
        0.0f, 1.0f, 0.0f,  // back left
        1.0f, 1.0f, 0.0f,  // back right
        1.0f, 1.0f, 1.0f,  // front right
        1.0f, 1.0f, 1.0f,  // front right
        0.0f, 1.0f, 1.0f,  // front left
        0.0f, 1.0f, 0.0f,  // back left
        // ===== bottom (y = -1) =====
        0.0f, 0.0f, 1.0f,  // front left
        1.0f, 0.0f, 1.0f,  // front right
        1.0f, 0.0f, 0.0f,  // back right
        1.0f, 0.0f, 0.0f,  // back right
        0.0f, 0.0f, 0.0f,  // back left
        0.0f, 0.0f, 1.0f   // front left
    };

constexpr float TEX_COORDS[6][6][2] = {
    // ===== front (z = +1) =====
    0.0f, 1.0f,  // bottom left
    0.0f, 0.0f,  // top left
    1.0f, 0.0f,  // top right
    1.0f, 0.0f,  // top right
    1.0f, 1.0f,  // bottom right
    0.0f, 1.0f,  // bottom left
    // ===== right (x = +1) =====
    0.0f, 1.0f,  // bottom front
    1.0f, 1.0f,  // bottom back
    1.0f, 0.0f,  // top back
    1.0f, 0.0f,  // top back
    0.0f, 0.0f,  // top front
    0.0f, 1.0f,  // bottom front
    // ===== back (z = -1) =====
    1.0f, 1.0f,  // bottom left
    0.0f, 1.0f,  // bottom right
    0.0f, 0.0f,  // top right
    0.0f, 0.0f,  // top right
    1.0f, 0.0f,  // top left
    1.0f, 1.0f,  // bottom left
    // ===== left (x = -1) =====
    1.0f, 1.0f,  // bottom back
    0.0f, 1.0f,  // bottom front
    0.0f, 0.0f,  // top front
    0.0f, 0.0f,  // top front
    1.0f, 0.0f,  // top back
    1.0f, 1.0f,  // bottom back
    // ===== top (y = +1) =====
    0.0f, 0.0f,  // back left
    1.0f, 0.0f,  // back right
    1.0f, 1.0f,  // front right
    1.0f, 1.0f,  // front right
    0.0f, 1.0f,  // front left
    0.0f, 0.0f,  // back left
    // ===== bottom (y = -1) =====
    0.0f, 0.0f,  // front left
    1.0f, 0.0f,  // front right
    1.0f, 1.0f,  // back right
    1.0f, 1.0f,  // back right
    0.0f, 1.0f,  // back left
    0.0f, 0.0f,  // front left
};

constexpr float CUBE_VER[24] = {
        0.0, 0.0, 0.0,
        1.0, 0.0, 0.0,
        1.0, 1.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
        1.0, 0.0, 1.0,
        1.0, 1.0, 1.0,
        0.0, 1.0, 1.0
    };


constexpr int OUTLINE_CUBE_INDICES[24] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

constexpr float SQUARE_VERTICES[6][2] = {
        -0.5f, -0.5f,  // bottom left
        -0.5f,  0.5f,  // top left
        0.5f,  0.5f,  // top right
        0.5f,  0.5f,  // top right
        0.5f, -0.5f,  // bottom right
        -0.5f, -0.5f   // bottom left
    };

constexpr float SQUARE_TEXTURE_POS[6][2] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
    };

struct Vertex {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float s = 0.0f, t = 0.0f;
    float layer = 0.0f;
};

struct Vertex2D {
    float x = 0.0f, y = 0.0f;
    float s = 0.0f, t = 0.0f;
    float layer = 0.0f;
};