#include "Cubed/gameplay/biome.hpp"

#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/perlin_noise.hpp"

#include <cmath>
#include <unordered_map>

namespace Cubed {

static PlainParams plain{{BiomeType::PLAIN,
                          {0.0f, 0.5f},
                          {0.0f, 0.5f},
                          {0.003f, 0.010f, 0.020f},
                          {62, 8}}};

static ForestParams forest{{BiomeType::FOREST,
                            {0.5f, 1.0f},
                            {0.5f, 1.0f},
                            {0.004f, 0.010f, 0.020f},
                            {62, 8}},
                           0.1f

};

static DesertParams desert{{BiomeType::DESERT,
                            {0.5f, 1.0f},
                            {0.0f, 0.5f},
                            {0.003f, 0.010f, 0.020f},
                            {61, 12}}};

static MountainParams mountain{{BiomeType::MOUNTAIN,
                                {0.0f, 0.5f},
                                {0.5f, 1.0f},
                                {0.006f, 0.014f, 0.010f},
                                {70, 70}}};

static RiverParams river{{BiomeType::RIVER,
                          {-0.1f, -0.1f},
                          {-0.1f, -0.1f},
                          {0.003f, 0.010f, 0.020f},
                          {50, 6}}};

std::string get_biome_str(BiomeType biome) {
    std::string str;
    using enum BiomeType;
    switch (biome) {
    case PLAIN:
        str = "Plain";
        break;
    case FOREST:
        str = "Forest";
        break;
    case DESERT:
        str = "Desert";
        break;
    case MOUNTAIN:
        str = "Mountain";
        break;
    case RIVER:
        str = "River";
        break;
    case NONE:
        str = "Unknown";
        break;
    }
    return str;
};
/*
Biome get_biome_from_noise(float temp, float humid) {
    auto weight = [](float t, float h, float ct, float ch) -> float {
            float dt = t - ct;
            float dh = h - ch;
            float dist = std::sqrt(dt*dt + dh*dh);
            return std::max(0.0f, 0.5f - dist);
        };
    float w_m = weight(temp, humid, 0.25f, 0.15f);
    float w_p = weight(temp, humid, 0.50f, 0.40f);
    float w_d = weight(temp, humid, 0.75f, 0.15f);
    float w_f = weight(temp, humid, 0.75f, 0.75f);
    w_m = pow(w_m, 8); w_p = pow(w_p, 8); w_d = pow(w_d, 8); w_f = pow(w_f, 8);
    if (w_m >= w_p && w_m >= w_d && w_m >= w_f) return Biome::MOUNTAIN;
    if (w_p >= w_m && w_p >= w_d && w_p >= w_f) return Biome::PLAIN;
    if (w_d >= w_m && w_d >= w_p && w_d >= w_f) return Biome::DESERT;
    return Biome::FOREST;
}
*/
BiomeType get_biome_from_noise(float temp, float humid) {
    using enum BiomeType;
    if (plain.temp.first <= temp && temp < plain.temp.second &&
        plain.humid.first <= humid && humid < plain.humid.second) {
        return PLAIN;
    }
    if (forest.temp.first <= temp && temp < forest.temp.second &&
        forest.humid.first <= humid && humid < forest.humid.second) {
        return FOREST;
    }
    if (desert.temp.first <= temp && temp < desert.temp.second &&
        desert.humid.first <= humid && humid < desert.humid.second) {
        return DESERT;
    }
    if (mountain.temp.first <= temp && temp <= mountain.temp.second &&
        mountain.humid.first <= humid && humid <= mountain.humid.second) {
        return MOUNTAIN;
    }
    Logger::warn("Invail Temp {} or Humid {}", temp, humid);
    return PLAIN;
}
std::array<float, 3> get_noise_frequencies_for_biome(BiomeType biome) {
    using enum BiomeType;
    switch (biome) {
    case PLAIN:
        return plain.frequencies;
    case FOREST:
        return forest.frequencies;
    case DESERT:
        return desert.frequencies;
    case MOUNTAIN:
        return mountain.frequencies;
    case RIVER:
        return river.frequencies;
    case NONE:
        ASSERT_MSG(false, "Chunk Biome is None");
        throw std::invalid_argument{"Chunk Biome is None"};
    }
    Logger::warn("Unknown Biome");
    return {0.003f, 0.015f, 0.06f};
}

BiomeHeightRange get_biome_height_range(BiomeType biome) {
    using enum BiomeType;
    switch (biome) {
    case PLAIN:
        return plain.height_range;
    case FOREST:
        return forest.height_range;
    case DESERT:
        return desert.height_range;
    case MOUNTAIN:
        return mountain.height_range;
    case RIVER:
        return river.height_range;
    case NONE:
        ASSERT_MSG(false, "Chunk Biome is None");
        throw std::invalid_argument{"Chunk Biome is None"};
    }
    Logger::warn("Unknown Biome");
    return {62, 4};
}

BiomeType safe_int_to_biome(int x) {
    using enum BiomeType;
    static const std::unordered_map<int, BiomeType> INT_TO_BIOME_MAP{
        {0, PLAIN}, {1, FOREST}, {2, DESERT}, {3, MOUNTAIN}, {4, RIVER}};

    auto it = INT_TO_BIOME_MAP.find(x);
    ASSERT_MSG(it != INT_TO_BIOME_MAP.end(), ":Can't Find");
    return it->second;
}

int get_interpolated_height(float world_x, float world_z, float temp,
                            float humid) {

    auto weight = [](float t, float h, float ct, float ch) -> float {
        float dt = t - ct;
        float dh = h - ch;
        float dist = std::sqrt(dt * dt + dh * dh);
        return std::max(0.0f, 0.5f - dist);
    };

    float w_mountain = weight(temp, humid, 0.25f, 0.15f);
    float w_plain = weight(temp, humid, 0.50f, 0.40f);
    float w_desert = weight(temp, humid, 0.75f, 0.15f);
    float w_forest = weight(temp, humid, 0.75f, 0.75f);
    // adjust transitions between chunks
    float pow_n = 8.0f; // the larger n is, the purer the biome
    w_mountain = std::pow(w_mountain, pow_n) * MOUNTAIN_FREQ;
    w_plain = std::pow(w_plain, pow_n) * PLAIN_FREQ;
    w_desert = std::pow(w_desert, pow_n) * DESERT_FREQ;
    w_forest = std::pow(w_forest, pow_n) * FOREST_FREQ;

    float total = w_mountain + w_plain + w_desert + w_forest;
    w_mountain /= total;
    w_plain /= total;
    w_desert /= total;
    w_forest /= total;

    auto sample_height = [&](BiomeType b) -> float {
        auto range = get_biome_height_range(b);
        auto [f1, f2, f3] = get_noise_frequencies_for_biome(b);
        float n = 1.00f * PerlinNoise::noise(world_x * f1, 0.5f, world_z * f1) +
                  0.50f * PerlinNoise::noise(world_x * f2, 0.5f, world_z * f2) +
                  0.25f * PerlinNoise::noise(world_x * f3, 0.5f, world_z * f3);
        n /= 1.75f;
        return range.base_y + n * range.amplitude;
    };

    float h = w_mountain * sample_height(BiomeType::MOUNTAIN) +
              w_plain * sample_height(BiomeType::PLAIN) +
              w_desert * sample_height(BiomeType::DESERT) +
              w_forest * sample_height(BiomeType::FOREST);
    return static_cast<int>(h);
}

PlainParams& plain_params() { return plain; }
ForestParams& forest_params() { return forest; }
DesertParams& desert_params() { return desert; }
MountainParams& mountain_params() { return mountain; }
RiverParams& river_params() { return river; }
} // namespace Cubed
