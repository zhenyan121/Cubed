#pragma once

#include <functional>

namespace Cubed {

struct ChunkPos {
    int x;
    int z;

    bool operator==(const ChunkPos&) const = default;
    struct Hash {
        std::size_t operator()(const ChunkPos& pos) const {
            std::size_t h1 = std::hash<int>{}(pos.x);
            std::size_t h2 = std::hash<int>{}(pos.z);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    ChunkPos operator+(const ChunkPos& pos) const {
        return ChunkPos{x + pos.x, z + pos.z};
    }

    ChunkPos& operator+=(const ChunkPos& pos) {
        x += pos.x;
        z += pos.z;
        return *this;
    };
};

} // namespace Cubed