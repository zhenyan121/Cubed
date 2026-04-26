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

enum class Biome {
    PLAIN = 0,
    FOREST,
    DESERT,
    MOUNTAIN,
    NONE
};


struct BiomeHeightRange {
    int base_y;
    int amplitude;
};

struct BiomeNonAdjacent {
    Biome first;
    std::vector<Biome> second;
    Biome replace;
};

static inline const std::vector<BiomeNonAdjacent> NON_ADJACENT {{
    {Biome::PLAIN, {Biome::NONE}, Biome::PLAIN},
    {Biome::FOREST, {Biome::DESERT}, Biome::PLAIN},
    {Biome::DESERT, {Biome::MOUNTAIN, Biome::FOREST}, Biome::PLAIN},
    {Biome::MOUNTAIN, {Biome::DESERT}, Biome::PLAIN}
}};

struct BaseBiomeParams {
    Biome biome;
    std::pair<float, float> temp;
    std::pair<float, float> humid;
    std::array<float, 3> frequencies;
    BiomeHeightRange height_range;
};

struct PlainParams : public BaseBiomeParams {

};

struct ForestParams : public BaseBiomeParams {
    float tree_frequency;
};

struct DesertParams : public BaseBiomeParams {

};

struct MountainParams : public BaseBiomeParams {

};

std::string get_biome_str(Biome biome);
Biome get_biome_from_noise(float temp, float humid);
std::array<float, 3> get_noise_frequencies_for_biome(Biome biome);
BiomeHeightRange get_biome_height_range(Biome biome);
Biome safe_int_to_biome(int x);
int get_interpolated_height(float world_x, float world_z, float temp, float humid);

PlainParams& plain_params();
ForestParams& forest_params();
DesertParams& desert_params();
MountainParams& mountain_params();

}
