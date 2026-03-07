#pragma once
#include <functional>
struct ChuckPos {
    int x;
    int y;
    bool operator==(const ChuckPos&) const = default;
    struct Hash {
        std::size_t operator()(const ChuckPos& pos) const{
            std::size_t h1 = std::hash<int>{}(pos.x);
            std::size_t h2 = std::hash<int>{}(pos.y);
            return h1 ^ (h2 << 1);
        }
    };
};

