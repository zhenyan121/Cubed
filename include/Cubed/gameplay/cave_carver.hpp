#pragma once
#include "Cubed/constants.hpp"
#include "Cubed/gameplay/path.hpp"

#include <tbb/concurrent_hash_map.h>

namespace Cubed {
class CaveCarver {

public:
    CaveCarver();

    void init(unsigned world_seed);
    void reload(unsigned world_seed);
    bool has_origin_fast(const ChunkPos& pos) const;
    float cave_probability() const;
    PathOrigin get_origin(const ChunkPos& origin_chunk) const;
    int search_radius() const;
    unsigned world_seed() const;

private:
    std::atomic<unsigned> m_world_seed{0};
    std::atomic<float> m_cave_probability{DEFAULT_CAVE_PROBABILITY};
};
} // namespace Cubed
