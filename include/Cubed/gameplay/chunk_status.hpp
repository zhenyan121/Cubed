#pragma once
#include <cmath>
#include <array>
#include <functional>
#include <string>

#include <Cubed/tools/log.hpp>
#include <Cubed/tools/cubed_assert.hpp>
struct ChunkPos {
    int x;
    int z;
    
    bool operator==(const ChunkPos&) const = default;
    struct Hash {
        std::size_t operator()(const ChunkPos& pos) const{
            std::size_t h1 = std::hash<int>{}(pos.x);
            std::size_t h2 = std::hash<int>{}(pos.z);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    ChunkPos operator+(const ChunkPos& pos) const{
        return ChunkPos{x + pos.x, z + pos.z};
    }

    ChunkPos& operator+=(const ChunkPos& pos) {
        x += pos.x;
        z += pos.z;
        return *this;
    };
};

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

constexpr inline std::string get_biome_str(Biome biome) {
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




