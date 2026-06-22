#pragma once
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/river.path.hpp"
#include "Cubed/tools/cubed_random.hpp"

#include <glm/glm.hpp>
#include <shared_mutex>
#include <tbb/concurrent_hash_map.h>

namespace Cubed {

class RiverWorm {
    using RiverHashMap =
        tbb::concurrent_hash_map<ChunkPos, RiverPath, ChunkPos::TBBHash>;

public:
    RiverWorm();
    ~RiverWorm();
    RiverHashMap& paths();
    void init(unsigned world_seed);
    void reload(unsigned world_seed);
    void add_path(const glm::vec3& pos, unsigned chunk_seed,
                  ChunkPos chunk_pos);
    void try_to_add_path(const ChunkPos& pos, unsigned chunk_seed);
    void cleanup_finished_rivers();

    int river_sum() const;
    float& river_probability();
    std::shared_mutex& paths_mutex();

private:
    RiverHashMap m_paths;
    unsigned m_seed = 0;
    Random m_random;
    float m_probability = 0.01f;
    std::shared_mutex m_paths_mutex;
};

}; // namespace Cubed