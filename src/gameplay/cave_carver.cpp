#include "Cubed/gameplay/cave_carver.hpp"

#include "Cubed/constants.hpp"

namespace Cubed {
CaveCarver::CaveCarver() {}

CaveCarver::CaveHashMap& CaveCarver::paths() { return m_paths; }

void CaveCarver::init(unsigned world_seed) {
    m_seed = world_seed;
    m_random.init(m_seed);
}

void CaveCarver::reload(unsigned world_seed) {
    m_seed = world_seed;
    m_paths.clear();
    init(world_seed);
}

void CaveCarver::add_path(const glm::vec3& pos, unsigned chunk_seed) {
    m_paths.emplace(chunk_seed, CavePath{chunk_seed, m_seed, pos});
}

void CaveCarver::try_to_add_path(const ChunkPos& chunk_pos,
                                 unsigned chunk_seed) {
    {
        CaveHashMap::const_accessor acc;
        if (m_paths.find(acc, chunk_seed)) {
            return;
        }
    }

    Random random{chunk_seed};
    if (random.random_bool(static_cast<double>(m_cave_probability))) {
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
        add_path(glm::vec3{x, y, z}, chunk_seed);
    }
}

void CaveCarver::cleanup_finished_caves() {
    std::vector<unsigned int> finished_keys;

    for (const auto& pair : m_paths) {
        if (pair.second.is_finished()) {
            finished_keys.push_back(pair.first);
        }
    }

    for (const auto& key : finished_keys) {
        m_paths.erase(key);
    }
}

int CaveCarver::cave_sum() const { return m_paths.size(); }
float& CaveCarver::cave_probability() { return m_cave_probability; }

} // namespace Cubed