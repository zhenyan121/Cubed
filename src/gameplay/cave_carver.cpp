#include "Cubed/gameplay/cave_carver.hpp"

#include "Cubed/constants.hpp"

namespace Cubed {
CaveCarver::CaveCarver() {}

std::unordered_map<int, CavePath>& CaveCarver::paths() { return m_paths; }

void CaveCarver::init(unsigned world_seed) {
    m_seed = world_seed;
    m_sum = 0;
    m_paths.emplace(m_sum,
                    CavePath{m_seed, m_sum, glm::vec3{0.0f, 20.0f, 0.0f}});
    m_sum++;
    m_random.init(m_seed);
}

void CaveCarver::reload(unsigned world_seed) {
    m_seed = world_seed;
    m_paths.clear();
    init(world_seed);
}

void CaveCarver::add_path(const glm::vec3& pos) {
    m_paths.emplace(m_sum, CavePath{m_seed, m_sum, pos});
    m_sum++;
}

void CaveCarver::try_to_add_path(const ChunkPos& chunk_pos,
                                 unsigned chunk_seed) {
    Random random{chunk_seed};
    if (random.random_bool(0.05)) {
        const int CHUNK_MIN_X = chunk_pos.x * CHUNK_SIZE;
        const int CHUNK_MIN_Z = chunk_pos.z * CHUNK_SIZE;
        const int CHUNK_MAX_X = CHUNK_MIN_X + SIZE_X - 1;
        const int CHUNK_MAX_Z = CHUNK_MIN_Z + SIZE_Z - 1;
        const int CHUNK_MIN_Y = 0;
        const int CHUNK_MAX_Y = SIZE_Y - 1;
        int max_y = std::min(CHUNK_MAX_Y, 40);
        int x = random.random_int(CHUNK_MIN_X, CHUNK_MAX_X);
        int y = random.random_int(CHUNK_MIN_Y + 1, max_y);
        int z = random.random_int(CHUNK_MIN_Z, CHUNK_MAX_Z);
        add_path(glm::vec3{x, y, z});
    }
}

void CaveCarver::cleanup_finished_caves() {
    std::erase_if(m_paths,
                  [](const auto& kv) { return kv.second.is_finished(); });
}

int CaveCarver::cave_sum() const { return m_sum; }

} // namespace Cubed