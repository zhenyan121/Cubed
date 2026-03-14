#pragma once
constexpr int WORLD_SIZE_X = 32;
constexpr int WORLD_SIZE_Z = 32;
constexpr int WORLD_SIZE_Y = 16;
constexpr int MAX_BLOCK_NUM = 2;
constexpr int CHUCK_SIZE = 16;
constexpr int DISTANCE = 8;
constexpr int MAX_BLOCK_STATUS = 1;

constexpr float VERTICES_POS[6][6][3] = {
        // ===== front (z = +1) =====
        -0.5f, -0.5f,  0.5f,  // bottom left
        -0.5f,  0.5f,  0.5f,  // top left
        0.5f,  0.5f,  0.5f,  // top right
        0.5f,  0.5f,  0.5f,  // top right
        0.5f, -0.5f,  0.5f,  // bottom right
        -0.5f, -0.5f,  0.5f,  // bottom left
        // ===== right (x = +1) =====
        0.5f, -0.5f,  0.5f,  // bottom front
        0.5f, -0.5f, -0.5f,  // bottom back
        0.5f,  0.5f, -0.5f,  // top back
        0.5f,  0.5f, -0.5f,  // top back
        0.5f,  0.5f,  0.5f,  // top front
        0.5f, -0.5f,  0.5f,  // bottom front
        // ===== back (z = -1) =====
        -0.5f, -0.5f, -0.5f,  // bottom left
        0.5f, -0.5f, -0.5f,  // bottom right
        0.5f,  0.5f, -0.5f,  // top right
        0.5f,  0.5f, -0.5f,  // top right
        -0.5f,  0.5f, -0.5f,  // top left
        -0.5f, -0.5f, -0.5f,  // bottom left
        // ===== left (x = -1) =====
        -0.5f, -0.5f, -0.5f,  // bottom back
        -0.5f, -0.5f,  0.5f,  // bottom front
        -0.5f,  0.5f,  0.5f,  // top front
        -0.5f,  0.5f,  0.5f,  // top front
        -0.5f,  0.5f, -0.5f,  // top back
        -0.5f, -0.5f, -0.5f,  // bottom back
        // ===== top (y = +1) =====
        -0.5f,  0.5f, -0.5f,  // back left
        0.5f,  0.5f, -0.5f,  // back right
        0.5f,  0.5f,  0.5f,  // front right
        0.5f,  0.5f,  0.5f,  // front right
        -0.5f,  0.5f,  0.5f,  // front left
        -0.5f,  0.5f, -0.5f,  // back left
        // ===== bottom (y = -1) =====
        -0.5f, -0.5f,  0.5f,  // front left
        0.5f, -0.5f,  0.5f,  // front right
        0.5f, -0.5f, -0.5f,  // back right
        0.5f, -0.5f, -0.5f,  // back right
        -0.5f, -0.5f, -0.5f,  // back left
        -0.5f, -0.5f,  0.5f   // front left
    };

constexpr float TEX_COORDS[6][6][2] = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

    };

constexpr float CUBE_VER[24] = {
        -0.5, -0.5, -0.5,
        0.5, -0.5, -0.5,
        0.5, 0.5, -0.5,
        -0.5, 0.5, -0.5,
        -0.5, -0.5, 0.5,
        0.5, -0.5, 0.5,
        0.5, 0.5, 0.5,
        -0.5, 0.5, 0.5
    };

constexpr int OUTLINE_CUBE_INDICES[24] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };