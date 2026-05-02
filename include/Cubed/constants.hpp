#pragma once
#include "Cubed/gameplay/chunk_pos.hpp"

#include <array>
namespace Cubed {

constexpr int WORLD_SIZE_Y = 256;
constexpr int CHUCK_SIZE = 16;
constexpr int SEA_LEVEL = 64;

constexpr int MAX_BLOCK_NUM = 8;
constexpr int MAX_UI_NUM = 1;
constexpr int MAX_BLOCK_STATUS = 1;
constexpr int MAX_BIOME_SUM = 4;
constexpr int MAX_CHARACTER = 128;

constexpr int PRE_LOAD_DISTANCE = 24;

constexpr int MAX_DISTANCE = 128;

constexpr float DEFAULT_FOV = 70.0f;
constexpr float DEFAULT_MAX_WALK_SPEED = 4.5f;
constexpr float DEFAULT_MAX_RUN_SPEED = 7.0f;
constexpr float DEFAULT_ACCELERATION = 10.0f;
constexpr float DEFAULT_DECELERATION = 15.0f;
constexpr float DEFAULT_G = 22.5f;
static constexpr int SIZE_X = CHUCK_SIZE;
static constexpr int SIZE_Y = WORLD_SIZE_Y;
static constexpr int SIZE_Z = CHUCK_SIZE;

constexpr ChunkPos CHUNK_DIR[]{{1, 0}, {-1, 0}, {0, 1},  {0, -1},
                               {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};

using HeightMapArray = std::array<std::array<int, CHUCK_SIZE>, CHUCK_SIZE>;

} // namespace Cubed