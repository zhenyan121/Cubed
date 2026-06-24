#pragma once

#include <array>
namespace Cubed {

constexpr int WORLD_SIZE_Y = 256;
constexpr int CHUNK_SIZE = 16;
constexpr int SEA_LEVEL = 63;

constexpr int MAX_UI_NUM = 1;
constexpr int MAX_BLOCK_STATUS = 1;
constexpr int MAX_BIOME_SUM = 4;
constexpr int MAX_CHARACTER = 128;

constexpr int PRE_LOAD_DISTANCE = 24;

constexpr int MAX_DISTANCE = 128;
constexpr int CROSS_PLANE_DISTANCE = 8;
constexpr float DEFAULT_FOV = 70.0f;
constexpr float DEFAULT_MAX_WALK_SPEED = 4.5f;
constexpr float DEFAULT_MAX_RUN_SPEED = 7.0f;
constexpr float DEFAULT_ACCELERATION = 10.0f;
constexpr float DEFAULT_DECELERATION = 15.0f;
constexpr float DEFAULT_G = 22.5f;
constexpr int SIZE_X = CHUNK_SIZE;
constexpr int SIZE_Y = WORLD_SIZE_Y;
constexpr int SIZE_Z = CHUNK_SIZE;
constexpr int RESERVED_THREADS = 5;

constexpr float DEFAULT_CAVE_PROBABILITY = 0.035f;

using HeightMapArray = std::array<std::array<int, CHUNK_SIZE>, CHUNK_SIZE>;

} // namespace Cubed