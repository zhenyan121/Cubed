#include "Cubed/gameplay/cave_carver.hpp"

#include "Cubed/constants.hpp"
#include "Cubed/gameplay/cave_path.hpp"
#include "Cubed/tools/cubed_hash.hpp"
#include "Cubed/tools/cubed_random.hpp"
namespace Cubed {
CaveCarver::CaveCarver() {}

void CaveCarver::init(unsigned world_seed) { m_world_seed = world_seed; }

void CaveCarver::reload(unsigned world_seed) {

    m_world_seed = world_seed;

    init(world_seed);
}

bool CaveCarver::has_origin_fast(const ChunkPos& pos) const {
    unsigned h = HASH::combine_32(HASH::combine_32(pos.x, pos.z), m_world_seed);

    return (h & 0xFFFF) < static_cast<unsigned>(m_cave_probability * 0xFFFF);
}

PathOrigin CaveCarver::get_origin(const ChunkPos& origin_chunk) const {
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
    const int CHUNK_MIN_Y = 0;
    const int CHUNK_MAX_Y = SIZE_Y - 1;
    int max_y = std::min(CHUNK_MAX_Y, 40);
    int x = random.random_int(CHUNK_MIN_X, CHUNK_MAX_X);
    int y = random.random_int(CHUNK_MIN_Y + 1, max_y);
    int z = random.random_int(CHUNK_MIN_Z, CHUNK_MAX_Z);
    return {true, {x, y, z}, chunk_seed};
}

int CaveCarver::search_radius() const {
    float max_displacement =
        3.0f * std::sqrt(static_cast<float>(CavePath::step_max())) *
        CavePath::step_len();
    return static_cast<int>(
        std::ceil((max_displacement + CavePath::radius_xz_max()) / CHUNK_SIZE));
}
unsigned CaveCarver::world_seed() const { return m_world_seed; }
float CaveCarver::cave_probability() const { return m_cave_probability; }
} // namespace Cubed