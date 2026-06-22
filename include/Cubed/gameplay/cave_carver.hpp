#pragma once
#include "Cubed/constants.hpp"
#include "Cubed/gameplay/cave_path.hpp"

#include <shared_mutex>
#include <tbb/concurrent_hash_map.h>

namespace Cubed {
class CaveCarver {
    using CaveHashMap =
        tbb::concurrent_hash_map<ChunkPos, CavePath, ChunkPos::TBBHash>;

public:
    CaveCarver();
    CaveHashMap& paths();
    void init(unsigned world_seed);
    void reload(unsigned world_seed);
    void add_path(const glm::vec3& pos, unsigned chunk_seed,
                  ChunkPos chunk_pos);
    void try_to_add_path(const ChunkPos& pos, unsigned chunk_seed);
    void cleanup_finished_caves();

    int cave_sum() const;
    float& cave_probability();
    std::shared_mutex& path_mutex();

private:
    CaveHashMap m_paths;
    unsigned m_seed = 0;
    Random m_random;
    float m_cave_probability = DEFAULT_CAVE_PROBABILITY;
    std::shared_mutex m_path_mutex;
};
} // namespace Cubed
