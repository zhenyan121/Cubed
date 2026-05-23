#pragma once
#include "Cubed/gameplay/cave_path.hpp"
namespace Cubed {
class CaveCarver {
public:
    CaveCarver();
    std::unordered_map<unsigned, CavePath>& paths();
    void init(unsigned world_seed);
    void reload(unsigned world_seed);
    void add_path(const glm::vec3& pos, unsigned chunk_seed);
    void try_to_add_path(const ChunkPos& pos, unsigned chunk_seed);
    void cleanup_finished_caves();

    int cave_sum() const;
    float& cave_probability();

private:
    std::unordered_map<unsigned, CavePath> m_paths;
    unsigned m_seed = 0;
    int m_sum = 0;
    Random m_random;
    float m_cave_probability = 0.035f;
};
} // namespace Cubed
