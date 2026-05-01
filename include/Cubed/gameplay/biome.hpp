#pragma once
#include <array>
#include <string>
#include <vector>

namespace Cubed {

constexpr float BIOME_NOISE_FREQUENCY = 0.03f;

constexpr float PLAIN_FREQ = 0.4f;
constexpr float FOREST_FREQ = 1.2f;
constexpr float DESERT_FREQ = 1.2f;
constexpr float MOUNTAIN_FREQ = 2.0f;

enum class BiomeType { PLAIN = 0, FOREST, DESERT, MOUNTAIN, RIVER, NONE };

struct BiomeHeightRange {
    int base_y;
    int amplitude;
};

struct BiomeNonAdjacent {
    BiomeType first;
    std::vector<BiomeType> second;
    BiomeType replace;
};

static inline const std::vector<BiomeNonAdjacent> NON_ADJACENT{
    {{BiomeType::PLAIN, {BiomeType::DESERT}, BiomeType::RIVER},
     {BiomeType::FOREST, {BiomeType::DESERT}, BiomeType::RIVER},
     {BiomeType::DESERT,
      {BiomeType::MOUNTAIN, BiomeType::FOREST},
      BiomeType::RIVER},
     {BiomeType::MOUNTAIN,
      {BiomeType::DESERT, BiomeType::FOREST},
      BiomeType::RIVER}}};

struct BaseBiomeParams {
    BiomeType biome;
    std::pair<float, float> temp;
    std::pair<float, float> humid;
    std::array<float, 3> frequencies;
    BiomeHeightRange height_range;
};

struct PlainParams : public BaseBiomeParams {};

struct ForestParams : public BaseBiomeParams {
    float tree_frequency;
};

struct DesertParams : public BaseBiomeParams {};

struct MountainParams : public BaseBiomeParams {};

struct RiverParams : public BaseBiomeParams {};

std::string get_biome_str(BiomeType biome);
BiomeType get_biome_from_noise(float temp, float humid);
std::array<float, 3> get_noise_frequencies_for_biome(BiomeType biome);
BiomeHeightRange get_biome_height_range(BiomeType biome);
BiomeType safe_int_to_biome(int x);
int get_interpolated_height(float world_x, float world_z, float temp,
                            float humid);

PlainParams& plain_params();
ForestParams& forest_params();
DesertParams& desert_params();
MountainParams& mountain_params();
RiverParams& river_params();
} // namespace Cubed
