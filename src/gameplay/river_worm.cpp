#include "Cubed/gameplay/river_worm.hpp"

#include "Cubed/constants.hpp"

namespace Cubed {
RiverWorm::RiverWorm() {}

RiverWorm::RiverHashMap& RiverWorm::paths() { return m_paths; }

void RiverWorm::init(unsigned world_seed) {
    m_seed = world_seed;

    m_random.init(m_seed);
}

void RiverWorm::reload(unsigned world_seed) {
    m_seed = world_seed;
    m_paths.clear();
    init(world_seed);
}

void RiverWorm::add_path(const glm::vec3& pos, unsigned chunk_seed) {
    m_paths.emplace(chunk_seed, RiverPath{chunk_seed, m_seed, pos});
}

void RiverWorm::try_to_add_path(const ChunkPos& chunk_pos,
                                unsigned chunk_seed) {
    {
        RiverHashMap::const_accessor acc;
        if (m_paths.find(acc, chunk_seed)) {
            return;
        }
    }
    Random random{chunk_seed};
    if (random.random_bool(static_cast<double>(m_probability))) {
        const int CHUNK_MIN_X = chunk_pos.x * CHUNK_SIZE;
        const int CHUNK_MIN_Z = chunk_pos.z * CHUNK_SIZE;
        const int CHUNK_MAX_X = CHUNK_MIN_X + SIZE_X - 1;
        const int CHUNK_MAX_Z = CHUNK_MIN_Z + SIZE_Z - 1;
        int x = random.random_int(CHUNK_MIN_X, CHUNK_MAX_X);
        int y = SEA_LEVEL + 2;
        int z = random.random_int(CHUNK_MIN_Z, CHUNK_MAX_Z);
        add_path(glm::vec3{x, y, z}, chunk_seed);
    }
}

void RiverWorm::cleanup_finished_rivers() {
    std::vector<unsigned> finished_keys;

    for (const auto& pair : m_paths) {
        if (pair.second.is_finished()) {
            finished_keys.push_back(pair.first);
        }
    }

    for (const auto& key : finished_keys) {
        m_paths.erase(key);
    }
}

int RiverWorm::river_sum() const { return m_paths.size(); }
float& RiverWorm::river_probability() { return m_probability; }

} // namespace Cubed