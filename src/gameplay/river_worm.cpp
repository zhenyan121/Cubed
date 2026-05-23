#include "Cubed/gameplay/river_worm.hpp"

#include "Cubed/constants.hpp"

namespace Cubed {
RiverWorm::RiverWorm() {}

std::unordered_map<unsigned, RiverPath>& RiverWorm::paths() { return m_paths; }

void RiverWorm::init(unsigned world_seed) {
    m_seed = world_seed;
    m_sum = 0;
    m_random.init(m_seed);
}

void RiverWorm::reload(unsigned world_seed) {
    m_seed = world_seed;
    m_paths.clear();
    init(world_seed);
}

void RiverWorm::add_path(const glm::vec3& pos, unsigned chunk_seed) {
    m_paths.emplace(chunk_seed, RiverPath{m_seed, m_sum, pos});
    m_sum++;
}

void RiverWorm::try_to_add_path(const ChunkPos& chunk_pos,
                                unsigned chunk_seed) {
    auto it = m_paths.find(chunk_seed);
    if (it != m_paths.end()) {
        return;
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
    std::erase_if(m_paths,
                  [](const auto& kv) { return kv.second.is_finished(); });
}

int RiverWorm::river_sum() const { return m_sum; }
float& RiverWorm::river_probability() { return m_probability; }
} // namespace Cubed