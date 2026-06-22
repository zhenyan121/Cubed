#pragma once
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/path.hpp"
#include "Cubed/tools/cubed_random.hpp"

#include <glm/glm.hpp>
#include <tbb/concurrent_hash_map.h>
namespace Cubed {

class RiverWorm {

public:
    RiverWorm();
    ~RiverWorm();

    void init(unsigned world_seed);
    void reload(unsigned world_seed);

    PathOrigin get_origin(const ChunkPos& origin_chunk) const;
    int search_radius() const;
    unsigned world_seed() const;

    float river_probability() const;
    bool has_origin_fast(const ChunkPos& pos) const;

private:
    std::atomic<unsigned> m_world_seed{0};
    Random m_random;
    std::atomic<float> m_probability{0.01f};
};

}; // namespace Cubed