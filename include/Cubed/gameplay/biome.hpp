#pragma once
#include <array>
#include <cmath>
#include <string>
#include <unordered_map>

#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/perlin_noise.hpp>

constexpr float BIOME_NOISE_FREQUENCY = 0.003f;

constexpr float PLAIN_FREQ = 0.5f;
constexpr float FOREST_FREQ = 1.0f;
constexpr float DESERT_FREQ = 1.0f;
constexpr float MOUNTAIN_FREQ = 2.0f;

enum class Biome {
    PLAIN = 0,
    FOREST,
    DESERT,
    MOUNTAIN
};

struct BiomeHeightRange {
    int base_y;
    int amplitude;
};

inline std::string get_biome_str(Biome biome) {
    std::string str;
    using enum Biome;
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
    }
    return str;
};

inline Biome get_biome_from_noise(float temp, float humid) {
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

inline std::array<float, 3> get_noise_frequencies_for_biome(Biome biome) {
    using enum Biome;
    switch (biome) {
        case PLAIN:
            return {0.003f, 0.010f, 0.020f};
        case FOREST:
            return {0.004f, 0.012f, 0.022f};
        case DESERT:
            return {0.003f, 0.010f, 0.020f};
        case MOUNTAIN:
            return {0.006f, 0.015f, 0.030f};
    }
    Logger::warn("Unknown Biome");
    return {0.003f, 0.015f, 0.06f};
}

inline BiomeHeightRange get_biome_height_range(Biome biome) {
    using enum Biome;
    switch (biome) {
        case PLAIN:
            return {62, 8};
        case FOREST:
            return {64, 12};
        case DESERT:
            return {61, 12};
        case MOUNTAIN:
            return {70, 70};
    }
    Logger::warn("Unknown Biome");
    return {62, 4};
}

inline Biome safe_int_to_biome(int x) {
    using enum Biome;
    static const std::unordered_map<int, Biome> INT_TO_BIOME_MAP {
        {0, PLAIN},
        {1, FOREST},
        {2, DESERT},
        {3, MOUNTAIN}
    };

    auto it = INT_TO_BIOME_MAP.find(x);
    CUBED_ASSERT_MSG(it != INT_TO_BIOME_MAP.end(), ":Can't Find");    
    return it->second;
}

inline int get_interpolated_height(float world_x, float world_z, float temp, float humid) {
        
        auto weight = [](float t, float h, float ct, float ch) -> float {
            float dt = t - ct;
            float dh = h - ch;
            float dist = std::sqrt(dt*dt + dh*dh);
            return std::max(0.0f, 0.5f - dist); 
        };

        float w_mountain = weight(temp, humid, 0.25f, 0.15f);
        float w_plain    = weight(temp, humid, 0.50f, 0.40f);
        float w_desert   = weight(temp, humid, 0.75f, 0.15f);
        float w_forest   = weight(temp, humid, 0.75f, 0.75f);
        // adjust transitions between chunks
        float pow_n = 8.0f; // the larger n is, the purer the biome
        w_mountain = std::pow(w_mountain, pow_n) * MOUNTAIN_FREQ;
        w_plain    = std::pow(w_plain,    pow_n) * PLAIN_FREQ;
        w_desert   = std::pow(w_desert,   pow_n) * DESERT_FREQ;
        w_forest   = std::pow(w_forest,   pow_n) * FOREST_FREQ;

        float total = w_mountain + w_plain + w_desert + w_forest;
        w_mountain /= total; w_plain /= total; w_desert /= total; w_forest /= total;

        auto sample_height = [&](Biome b) -> float {
            auto range = get_biome_height_range(b);
            auto [f1, f2, f3] = get_noise_frequencies_for_biome(b);
            float n =
                1.00f * PerlinNoise::noise(world_x * f1, 0.5f, world_z * f1) +
                0.50f * PerlinNoise::noise(world_x * f2, 0.5f, world_z * f2) +
                0.25f * PerlinNoise::noise(world_x * f3, 0.5f, world_z * f3);
            n /= 1.75f;
            return range.base_y + n * range.amplitude;
        };

        float h = w_mountain * sample_height(Biome::MOUNTAIN)
                + w_plain    * sample_height(Biome::PLAIN)
                + w_desert   * sample_height(Biome::DESERT)
                + w_forest   * sample_height(Biome::FOREST);
        return static_cast<int>(h);
    }


