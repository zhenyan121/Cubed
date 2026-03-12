#pragma once
#include <functional>
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
};

struct Vertex {
    float x, y, z;
    float s, t;
    float layer;
};
