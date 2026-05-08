#pragma once
#include <cstdint>
#include <string_view>
namespace Cubed {

namespace HASH {
inline std::size_t str(std::string_view value) {
    return std::hash<std::string_view>{}(value);
}
inline uint32_t mix_hash(int32_t a, int32_t b, uint32_t fixed_seed) {
    uint32_t h = fixed_seed;

    h ^= (uint32_t)a * 0xcc9e2d51u;
    h = (h << 15) | (h >> 17); // rotl 15
    h *= 0x1b873593u;

    h ^= (uint32_t)b * 0xcc9e2d51u;
    h = (h << 15) | (h >> 17); // rotl 15
    h *= 0x1b873593u;

    // Finalization（avalanche）
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;

    return h;
}
inline uint32_t combine_32(uint32_t seed, uint32_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}
} // namespace HASH

} // namespace Cubed