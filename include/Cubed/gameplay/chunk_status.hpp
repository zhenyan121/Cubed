#pragma once
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
    if (temp < 0.5f && humid < 0.5f) return Biome::MOUNTAIN;
    if (temp < 0.5f && humid >= 0.5f) return Biome::PLAIN;
    if (temp >= 0.5f && humid < 0.5f) return Biome::DESERT;
    return Biome::FOREST;
}

constexpr inline std::array<float, 3> get_noise_frequencies_for_biome(Biome biome) {
    using enum Biome;
    switch (biome) {
        case PLAIN:
            return {0.003f, 0.008f, 0.018f};
        case FOREST:
            return {0.004f, 0.012f, 0.022f};
        case DESERT:
            return {0.003f, 0.008f, 0.018f};
        case MOUNTAIN:
            return {0.006f, 0.015f, 0.03f};
    }
    Logger::warn("Unknown Biome");
    return {0.003f, 0.015f, 0.06f};
}

constexpr inline BiomeHeightRange get_biome_height_range(Biome biome) {
    using enum Biome;
    switch (biome) {
        case PLAIN:
            return {62, 4};
        case FOREST:
            return {64, 8};
        case DESERT:
            return {61, 8};
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




