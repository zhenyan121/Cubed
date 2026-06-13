#pragma once
#include "Cubed/gameplay/cave_path.hpp"

#include <tbb/concurrent_hash_map.h>
namespace Cubed {
class CaveCarver {
    using CaveHashMap = tbb::concurrent_hash_map<unsigned, CavePath>;

public:
    CaveCarver();
    CaveHashMap& paths();
    void init(unsigned world_seed);
    void reload(unsigned world_seed);
    void add_path(const glm::vec3& pos, unsigned chunk_seed);
    void try_to_add_path(const ChunkPos& pos, unsigned chunk_seed);
    void cleanup_finished_caves();

    int cave_sum() const;
    float& cave_probability();

private:
    CaveHashMap m_paths;
    unsigned m_seed = 0;
    int m_sum = 0;
    Random m_random;
    float m_cave_probability = 0.035f;
};
} // namespace Cubed
