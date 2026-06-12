#pragma once
#include <array>
#include <string>
#include <vector>

namespace Cubed {

constexpr float BIOME_NOISE_FREQUENCY = 0.06f;
constexpr float HEIGHTMAP_NOISE_FREQUENCY = 0.001f;
constexpr float MOUNTAINOUS_NOISE_FREQUENCY = 0.003f;
enum class BiomeType {
    PLAIN = 0,
    FOREST,
    DESERT,
    MOUNTAIN,
    RIVER,
    SNOWY_PLAIN,
    OCEAN,
    NONE
};

struct BiomeConditions {
    float temp = 0.0f;
    float humid = 0.0f;
    float mountainous = 0.0f;
};

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
     {BiomeType::DESERT, {BiomeType::FOREST}, BiomeType::RIVER},
     {BiomeType::MOUNTAIN, {BiomeType::NONE}, BiomeType::RIVER}}};

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
// std::array<float, 3> get_noise_frequencies_for_biome(BiomeType biome);
// BiomeHeightRange get_biome_height_range(BiomeType biome);
BiomeType safe_int_to_biome(int x);
int get_interpolated_height(float world_x, float world_z, float temp,
                            float humid);

BiomeType determine_biome(const BiomeConditions& conditions);

PlainParams& plain_params();
ForestParams& forest_params();
DesertParams& desert_params();
MountainParams& mountain_params();
RiverParams& river_params();

} // namespace Cubed
