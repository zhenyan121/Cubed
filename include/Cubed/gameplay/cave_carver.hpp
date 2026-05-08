#pragma once
#include "Cubed/gameplay/cave_path.hpp"
namespace Cubed {
class CaveCarver {
public:
    CaveCarver();
    std::unordered_map<int, CavePath>& paths();
    void init(unsigned world_seed);
    void reload(unsigned world_seed);
    void add_path(const glm::vec3& pos);
    void try_to_add_path(const ChunkPos& pos);
    void cleanup_finished_caves();

    int cave_sum() const;

private:
    std::unordered_map<int, CavePath> m_paths;
    unsigned m_seed = 0;
    int m_sum = 0;
    Random m_random;
};
} // namespace Cubed
