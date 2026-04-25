#pragma once
#include <array>
namespace Cubed {

constexpr int WORLD_SIZE_Y = 256;
constexpr int CHUCK_SIZE = 16;

constexpr int MAX_BLOCK_NUM = 7;
constexpr int MAX_UI_NUM = 1;
constexpr int MAX_BLOCK_STATUS = 1;
constexpr int MAX_BIOME_SUM = 4;
constexpr int MAX_CHARACTER = 128;

constexpr int PRE_LOAD_DISTANCE = 24;

constexpr int MAX_DISTANCE = 128;

constexpr float DEFAULT_FOV = 70.0f;

using HeightMapArray = std::array<std::array<float, CHUCK_SIZE>, CHUCK_SIZE>;

}