#include "Cubed/gameplay/river_worm.hpp"

#include "Cubed/constants.hpp"
#include "Cubed/gameplay/river.path.hpp"
#include "Cubed/tools/cubed_hash.hpp"
namespace Cubed {
RiverWorm::RiverWorm() {}
RiverWorm::~RiverWorm() {}

void RiverWorm::init(unsigned world_seed) {
    m_world_seed = world_seed;

    m_random.init(m_world_seed);
}

void RiverWorm::reload(unsigned world_seed) {

    m_world_seed = world_seed;

    init(world_seed);
}

bool RiverWorm::has_origin_fast(const ChunkPos& pos) const {
    unsigned h = HASH::combine_32(HASH::combine_32(pos.x, pos.z), m_world_seed);

    return (h & 0xFFFF) < static_cast<unsigned>(m_probability * 0xFFFF);
}

PathOrigin RiverWorm::get_origin(const ChunkPos& origin_chunk) const {
    // Quickly check if there is an origin point without constructing Random
    if (!has_origin_fast(origin_chunk)) {
        return {false, {}, 0};
    }

    unsigned chunk_seed =
        HASH::chunk_seed_hash(origin_chunk.x, origin_chunk.z, m_world_seed);
    Random random{chunk_seed};

    const int CHUNK_MIN_X = origin_chunk.x * CHUNK_SIZE;
    const int CHUNK_MIN_Z = origin_chunk.z * CHUNK_SIZE;
    const int CHUNK_MAX_X = CHUNK_MIN_X + SIZE_X - 1;
    const int CHUNK_MAX_Z = CHUNK_MIN_Z + SIZE_Z - 1;
    int x = random.random_int(CHUNK_MIN_X, CHUNK_MAX_X);
    int y = SEA_LEVEL + 2;
    int z = random.random_int(CHUNK_MIN_Z, CHUNK_MAX_Z);
    return {true, {x, y, z}, chunk_seed};
}

int RiverWorm::search_radius() const {
    float max_displacement =
        3.0f * std::sqrt(static_cast<float>(RiverPath::step_max())) *
        RiverPath::step_len();
    return static_cast<int>(std::ceil(
        (max_displacement + RiverPath::radius_xz_max()) / CHUNK_SIZE));
}
unsigned RiverWorm::world_seed() const { return m_world_seed; }
float RiverWorm::river_probability() const { return m_probability; }

} // namespace Cubed