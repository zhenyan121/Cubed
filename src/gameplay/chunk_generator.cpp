#include "Cubed/gameplay/chunk_generator.hpp"

#include "Cubed/gameplay/builders/desert_builder.hpp"
#include "Cubed/gameplay/builders/forest_builder.hpp"
#include "Cubed/gameplay/builders/mountain_builder.hpp"
#include "Cubed/gameplay/builders/plain_builder.hpp"
#include "Cubed/gameplay/builders/river_builder.hpp"
#include "Cubed/gameplay/chunk.hpp"
#include "Cubed/gameplay/tree.hpp"
#include "Cubed/gameplay/world.hpp"
#include "Cubed/tools/cubed_hash.hpp"
#include "Cubed/tools/perlin_noise.hpp"
namespace Cubed {

using enum BiomeType;

constexpr int BLEND_RADIUS = 12;
ChunkGenerator::ChunkGenerator(Chunk& chunk) : m_chunk(chunk) {
    ASSERT_MSG(is_init, "ChunksGenerator is not init");
    ChunkPos pos = m_chunk.get_chunk_pos();
    unsigned seed = HASH::mix_hash(pos.x, pos.z, m_generator_seed);
    m_random.init(seed);
}

void ChunkGenerator::init() {
    std::random_device d;
    m_generator_seed = d();
    Logger::info("Chunk Generator Seed {}", m_generator_seed);
    PerlinNoise::init(m_generator_seed);
    is_init = true;
}

void ChunkGenerator::reload() {
    if (!is_seed_change) {
        return;
    }
    PerlinNoise::reload(m_generator_seed);
    is_seed_change = false;
}

const unsigned& ChunkGenerator::seed() { return m_generator_seed; }
void ChunkGenerator::seed(unsigned s) {
    is_seed_change = true;
    m_generator_seed = s;
}

void ChunkGenerator::assign_chunk_biome() {
    auto m_chunk_pos = m_chunk.chunk_pos();
    float x = static_cast<float>(m_chunk_pos.x);
    float z = static_cast<float>(m_chunk_pos.z);
    float temp = PerlinNoise::noise(x * BIOME_NOISE_FREQUENCY, 0.0f,
                                    z * BIOME_NOISE_FREQUENCY);
    float humid = PerlinNoise::noise(x * BIOME_NOISE_FREQUENCY, 1.0f,
                                     z * BIOME_NOISE_FREQUENCY);
    auto biome = get_biome_from_noise(temp, humid);
    m_chunk.biome(biome);
}

void ChunkGenerator::resolve_biome_adjacency_conflict(
    const std::array<const Chunk*, 8>& adj_chunks) {
    auto m_biome = m_chunk.biome();
    for (int i = 0; i < 8; i++) {
        auto& chunk = adj_chunks[i];
        if (chunk == nullptr) {
            continue;
        }
        BiomeType biome = chunk->get_biome();
        for (const auto& non : NON_ADJACENT) {
            if (m_biome != non.first) {
                continue;
            }
            for (auto b : non.second) {
                if (b == biome) {
                    m_biome = non.replace;
                    m_chunk.biome(m_biome);
                    return;
                }
            }
        }
    }
}

void ChunkGenerator::generate_heightmap() {

    auto m_chunk_pos = m_chunk.chunk_pos();
    auto& m_heightmap = m_chunk.heightmap();
    auto m_biome = m_chunk.biome();

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {

            float world_x = static_cast<float>(x + m_chunk_pos.x * CHUNK_SIZE);
            float world_z = static_cast<float>(z + m_chunk_pos.z * CHUNK_SIZE);

            auto sample_height = [&](BiomeType b) -> int {
                auto range = get_biome_height_range(b);
                auto [f1, f2, f3] = get_noise_frequencies_for_biome(b);
                float n = 1.00f * PerlinNoise::noise(world_x * f1, 0.5f,
                                                     world_z * f1) +
                          0.50f * PerlinNoise::noise(world_x * f2, 0.5f,
                                                     world_z * f2) +
                          0.25f * PerlinNoise::noise(world_x * f3, 0.5f,
                                                     world_z * f3);
                n /= 1.75f;
                return range.base_y + std::round(n * range.amplitude);
            };
            m_heightmap[x][z] = sample_height(m_biome);
        }
    }
}

void ChunkGenerator::blend_heightmap_boundaries(
    const std::array<std::optional<HeightMapArray>, 8>& neighbor_heightmap,
    const std::array<BiomeType, 8>& neighbor_biome) {
    auto& m_heightmap = m_chunk.heightmap();
    auto m_biome = m_chunk.biome();
    m_neighbor_biome = neighbor_biome;
    // --- Right neighbor neighbor[0]: (1, 0) ---
    for (int z = 0; z < SIZE_Z; z++) {
        if (neighbor_heightmap[0] != std::nullopt &&
            neighbor_biome[0] != m_biome) {
            is_cur_chunk_ins = true;
            int edge_x = CHUNK_SIZE - 1;
            int h = m_heightmap[edge_x][z];
            int neighbor_h = (*neighbor_heightmap[0])[0][z];
            if (h <= neighbor_h) {
                continue;
            }
            const int DIR = (edge_x == 0) ? 1 : -1;
            for (int i = 0; i < BLEND_RADIUS; i++) {
                int x = edge_x + DIR * i;
                float t = static_cast<float>(i) / BLEND_RADIUS;

                // float smooth_t = t * t * (3.0f - 2.0f * t);
                float smooth_t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                m_heightmap[x][z] = static_cast<int>(
                    std::round(neighbor_h + (h - neighbor_h) * smooth_t));
            }
        }
    }
    // --- Left neighbor neighbor[1]: (-1, 0) ---
    for (int z = 0; z < SIZE_Z; z++) {
        if (neighbor_heightmap[1] != std::nullopt &&
            neighbor_biome[1] != m_biome) {
            is_cur_chunk_ins = true;
            int edge_x = 0;
            int h = m_heightmap[edge_x][z];
            int neighbor_h = (*neighbor_heightmap[1])[CHUNK_SIZE - 1][z];
            if (h <= neighbor_h) {
                continue;
            }

            const int DIR = (edge_x == 0) ? 1 : -1;
            for (int i = 0; i < BLEND_RADIUS; i++) {
                int x = edge_x + DIR * i;
                float t = static_cast<float>(i) / BLEND_RADIUS;

                // float smooth_t = t * t * (3.0f - 2.0f * t);
                float smooth_t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                m_heightmap[x][z] = static_cast<int>(
                    std::round(neighbor_h + (h - neighbor_h) * smooth_t));
            }
        }
    }
    // --- Front neighbor neighbor[2]: (0, 1) ---
    for (int x = 0; x < SIZE_X; x++) {
        if (neighbor_heightmap[2] != std::nullopt &&
            neighbor_biome[2] != m_biome) {
            is_cur_chunk_ins = true;
            int edge_z = CHUNK_SIZE - 1;
            int h = m_heightmap[x][edge_z];
            int neighbor_h = (*neighbor_heightmap[2])[x][0];
            if (h <= neighbor_h) {
                continue;
            }
            const int DIR = (edge_z == 0) ? 1 : -1;
            for (int i = 0; i < BLEND_RADIUS; i++) {
                int z = edge_z + DIR * i;
                float t = static_cast<float>(i) / BLEND_RADIUS;

                // float smooth_t = t * t * (3.0f - 2.0f * t);
                float smooth_t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                m_heightmap[x][z] = static_cast<int>(
                    std::round(neighbor_h + (h - neighbor_h) * smooth_t));
            }
        }
    }

    // --- Back neighbor neighbor[3]: (0, -1) ---
    for (int x = 0; x < SIZE_X; x++) {
        if (neighbor_heightmap[3] != std::nullopt &&
            neighbor_biome[3] != m_biome) {
            is_cur_chunk_ins = true;
            int edge_z = 0;
            int h = m_heightmap[x][edge_z];
            int neighbor_h = (*neighbor_heightmap[3])[x][CHUNK_SIZE - 1];
            if (h <= neighbor_h) {
                continue;
            }

            const int DIR = (edge_z == 0) ? 1 : -1;
            for (int i = 0; i < BLEND_RADIUS; i++) {
                int z = edge_z + DIR * i;
                float t = static_cast<float>(i) / BLEND_RADIUS;

                // float smooth_t = t * t * (3.0f - 2.0f * t);
                float smooth_t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                m_heightmap[x][z] = static_cast<int>(
                    std::round(neighbor_h + (h - neighbor_h) * smooth_t));
            }
        }
    }

    if (is_cur_chunk_ins) {
        return;
    }
    // --- Right-Front corner neighbor[4]: (1, 1) ---
    if (neighbor_heightmap[4] != std::nullopt && neighbor_biome[4] != m_biome) {
        for (int i = 0; i < BLEND_RADIUS; i++) {
            for (int j = 0; j < BLEND_RADIUS; j++) {
                int x = (CHUNK_SIZE - 1) - i;
                int z = (CHUNK_SIZE - 1) - j;
                int h = m_heightmap[x][z];

                int h_right = (neighbor_heightmap[0] != std::nullopt)
                                  ? (*neighbor_heightmap[0])[0][z]
                                  : h;

                int h_front = (neighbor_heightmap[2] != std::nullopt)
                                  ? (*neighbor_heightmap[2])[x][0]
                                  : h;

                int h_corner = (*neighbor_heightmap[4])[0][0];

                float tx = static_cast<float>(i) / BLEND_RADIUS;
                float tz = static_cast<float>(j) / BLEND_RADIUS;

                float target_h = h_corner * (1 - tx) * (1 - tz) +
                                 h_front * tx * (1 - tz) +
                                 h_right * (1 - tx) * tz + h * tx * tz;

                if (h <= static_cast<int>(std::round(target_h)))
                    continue;

                float t = static_cast<float>(std::max(i, j)) / BLEND_RADIUS;
                float smooth_t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                m_heightmap[x][z] = static_cast<int>(
                    std::round(target_h + (h - target_h) * smooth_t));
            }
        }
    }

    // --- Left-Front corner neighbor[5]: (-1, 1) ---
    if (neighbor_heightmap[5] != std::nullopt && neighbor_biome[5] != m_biome) {
        for (int i = 0; i < BLEND_RADIUS; i++) {
            for (int j = 0; j < BLEND_RADIUS; j++) {
                int x = i;
                int z = (CHUNK_SIZE - 1) - j;
                int h = m_heightmap[x][z];
                int h_left = (neighbor_heightmap[1] != std::nullopt)
                                 ? (*neighbor_heightmap[1])[CHUNK_SIZE - 1][z]
                                 : h;
                int h_front = (neighbor_heightmap[2] != std::nullopt)
                                  ? (*neighbor_heightmap[2])[x][0]
                                  : h;
                int h_corner = (*neighbor_heightmap[5])[CHUNK_SIZE - 1][0];

                float tx = static_cast<float>(i) / BLEND_RADIUS;
                float tz = static_cast<float>(j) / BLEND_RADIUS;
                float target_h = h_corner * (1 - tx) * (1 - tz) +
                                 h_front * tx * (1 - tz) +
                                 h_left * (1 - tx) * tz + h * tx * tz;

                if (h <= static_cast<int>(std::round(target_h)))
                    continue;

                float t = static_cast<float>(std::max(i, j)) / BLEND_RADIUS;
                float smooth_t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                m_heightmap[x][z] = static_cast<int>(
                    std::round(target_h + (h - target_h) * smooth_t));
            }
        }
    }

    // --- Right-Back corner neighbor[6]: (1, -1) ---
    if (neighbor_heightmap[6] != std::nullopt && neighbor_biome[6] != m_biome) {
        for (int i = 0; i < BLEND_RADIUS; i++) {
            for (int j = 0; j < BLEND_RADIUS; j++) {
                int x = (CHUNK_SIZE - 1) - i;
                int z = j;
                int h = m_heightmap[x][z];
                int h_right = (neighbor_heightmap[0] != std::nullopt)
                                  ? (*neighbor_heightmap[0])[0][z]
                                  : h;
                int h_back = (neighbor_heightmap[3] != std::nullopt)
                                 ? (*neighbor_heightmap[3])[x][CHUNK_SIZE - 1]
                                 : h;
                int h_corner = (*neighbor_heightmap[6])[0][CHUNK_SIZE - 1];

                float tx = static_cast<float>(i) / BLEND_RADIUS;
                float tz = static_cast<float>(j) / BLEND_RADIUS;
                float target_h = h_corner * (1 - tx) * (1 - tz) +
                                 h_back * tx * (1 - tz) +
                                 h_right * (1 - tx) * tz + h * tx * tz;

                if (h <= static_cast<int>(std::round(target_h)))
                    continue;

                float t = static_cast<float>(std::max(i, j)) / BLEND_RADIUS;
                float smooth_t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                m_heightmap[x][z] = static_cast<int>(
                    std::round(target_h + (h - target_h) * smooth_t));
            }
        }
    }

    // --- Left-Back corner neighbor[7]: (-1, -1) ---
    if (neighbor_heightmap[7] != std::nullopt && neighbor_biome[7] != m_biome) {
        for (int i = 0; i < BLEND_RADIUS; i++) {
            for (int j = 0; j < BLEND_RADIUS; j++) {
                int x = i;
                int z = j;
                int h = m_heightmap[x][z];
                int h_left = (neighbor_heightmap[1] != std::nullopt)
                                 ? (*neighbor_heightmap[1])[CHUNK_SIZE - 1][z]
                                 : h;
                int h_back = (neighbor_heightmap[3] != std::nullopt)
                                 ? (*neighbor_heightmap[3])[x][CHUNK_SIZE - 1]
                                 : h;
                int h_corner =
                    (*neighbor_heightmap[7])[CHUNK_SIZE - 1][CHUNK_SIZE - 1];

                float tx = static_cast<float>(i) / BLEND_RADIUS;
                float tz = static_cast<float>(j) / BLEND_RADIUS;
                float target_h = h_corner * (1 - tx) * (1 - tz) +
                                 h_back * tx * (1 - tz) +
                                 h_left * (1 - tx) * tz + h * tx * tz;

                if (h <= static_cast<int>(std::round(target_h)))
                    continue;

                float t = static_cast<float>(std::max(i, j)) / BLEND_RADIUS;
                float smooth_t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                m_heightmap[x][z] = static_cast<int>(
                    std::round(target_h + (h - target_h) * smooth_t));
            }
        }
    }
}

void ChunkGenerator::generate_terrain_blocks() {
    make_biome_builder();
    if (!m_biome_builder) {
        Logger::error("BiomeBuilder is nullptr");
        return;
    }
    m_chunk.blocks().assign(CHUNK_SIZE * CHUNK_SIZE * WORLD_SIZE_Y, 0);
    m_biome_builder->build_biome();
    generate_cave();
}

void ChunkGenerator::blend_surface_blocks_borders(
    const std::array<std::optional<std::vector<uint8_t>>, 4>& neighbor_block) {
    auto& m_blocks = m_chunk.blocks();
    auto& m_heightmap = m_chunk.heightmap();

    constexpr int WORLD_HEIGHT = WORLD_SIZE_Y;

    // Helper lambda: get top block type from a neighbor's block data at (nx,
    // nz)
    auto get_top_block_from_neighbor = [&](const std::vector<uint8_t>& blocks,
                                           int nx, int nz) -> uint8_t {
        // Search from topmost y downwards for the first non-zero block
        for (int y = WORLD_HEIGHT - 1; y >= 0; --y) {
            int idx = Chunk::get_index(
                nx, y, nz); // linear index: y * area + z * size + x
            if (idx >= 0 && idx < static_cast<int>(blocks.size()) &&
                blocks[idx] != 0) {
                return blocks[idx];
            }
        }
        return 0; // fallback, should not happen for valid chunks
    };

    // For each column (x, z)
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            // Get the current top block type of this column from m_blocks
            uint8_t type_self = 0;
            int top_y = -1;
            top_y = m_heightmap[x][z];
            type_self = m_blocks[Chunk::get_index(x, top_y, z)];

            if (top_y == -1)
                continue; // no block? skip

            // Weight map: type -> total weight
            std::unordered_map<uint8_t, float> weights;
            weights[type_self] = 1.0f; // self weight

            // --- Right neighbor (index 0) ---
            if (neighbor_block[0] && x >= CHUNK_SIZE - BLEND_RADIUS) {
                int dist = (CHUNK_SIZE - 1) - x;
                float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                t = t * t * (3.0f - 2.0f * t); // smoothstep
                if (t > 0.0f) {
                    uint8_t type_neighbor =
                        get_top_block_from_neighbor(*neighbor_block[0], 0, z);
                    weights[type_neighbor] += t;
                }
            }

            // --- Left neighbor (index 1) ---
            if (neighbor_block[1] && x < BLEND_RADIUS) {
                int dist = x;
                float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                t = t * t * (3.0f - 2.0f * t);
                if (t > 0.0f) {
                    uint8_t type_neighbor = get_top_block_from_neighbor(
                        *neighbor_block[1], CHUNK_SIZE - 1, z);
                    weights[type_neighbor] += t;
                }
            }

            // --- Front neighbor (index 2) ---
            if (neighbor_block[2] && z >= CHUNK_SIZE - BLEND_RADIUS) {
                int dist = (CHUNK_SIZE - 1) - z;
                float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                t = t * t * (3.0f - 2.0f * t);
                if (t > 0.0f) {
                    uint8_t type_neighbor =
                        get_top_block_from_neighbor(*neighbor_block[2], x, 0);
                    weights[type_neighbor] += t;
                }
            }

            // --- Back neighbor (index 3) ---
            if (neighbor_block[3] && z < BLEND_RADIUS) {
                int dist = z;
                float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                t = t * t * (3.0f - 2.0f * t);
                if (t > 0.0f) {
                    uint8_t type_neighbor = get_top_block_from_neighbor(
                        *neighbor_block[3], x, CHUNK_SIZE - 1);
                    weights[type_neighbor] += t;
                }
            }

            // Find type with maximum total weight
            uint8_t final_type = type_self;
            float max_weight = weights[type_self];
            for (const auto& [type, w] : weights) {
                if (w > max_weight) {
                    max_weight = w;
                    final_type = type;
                }
            }

            // Update the top block if the type changed
            if (final_type != type_self) {
                // top block
                if (m_chunk.biome() == BiomeType::RIVER && final_type == 1) {
                    final_type = 2;
                }

                m_blocks[Chunk::get_index(x, top_y, z)] = final_type;
                // bottom block
                unsigned fill_type = 2;
                if (final_type == 1) {
                    fill_type = 2;
                } else if (final_type == 4) {
                    fill_type = 4;
                }
                for (int y = top_y - 5; y < top_y; y++) {
                    m_blocks[Chunk::get_index(x, y, z)] = fill_type;
                }
            }
        }
    }
}

void ChunkGenerator::generate_vegetation() {

    if (!m_biome_builder) {
        Logger::error("BiomeBuilder is nullptr");
        return;
    }
    m_biome_builder->build_vegetation();
}

void ChunkGenerator::make_biome_builder() {
    auto biome = m_chunk.biome();
    switch (biome) {
    case PLAIN:
        m_biome_builder = std::make_unique<PlainBuilder>(*this);
        break;
    case DESERT:
        m_biome_builder = std::make_unique<DesertBuilder>(*this);
        break;
    case FOREST:
        m_biome_builder = std::make_unique<ForestBuilder>(*this);
        break;
    case MOUNTAIN:
        m_biome_builder = std::make_unique<MountainBuilder>(*this);
        break;
    case RIVER:
        m_biome_builder = std::make_unique<RiverBuilder>(*this);
        break;
    case NONE:
        m_biome_builder = nullptr;
        break;
    }
}

void ChunkGenerator::generate_cave() {
    auto& cave_carver = m_chunk.world().cave_carcer();
    auto& paths = cave_carver.paths();
    const auto& chunk_pos = m_chunk.chunk_pos();
    auto& blocks = m_chunk.blocks();
    const int CHUNK_MIN_X = chunk_pos.x * CHUNK_SIZE;
    const int CHUNK_MIN_Z = chunk_pos.z * CHUNK_SIZE;
    const int CHUNK_MAX_X = CHUNK_MIN_X + SIZE_X - 1;
    const int CHUNK_MAX_Z = CHUNK_MIN_Z + SIZE_Z - 1;
    const int CHUNK_MIN_Y = 0;
    const int CHUNK_MAX_Y = SIZE_Y - 1;
    for (auto& [id, path] : paths) {
        for (const auto& point : path.points()) {

            const glm::vec3& center = point.pos;
            float rad_xz = point.rad_xz;
            float rad_y = point.rad_y;

            int min_x = static_cast<int>(std::floor(center.x - rad_xz));
            int max_x = static_cast<int>(std::floor(center.x + rad_xz));
            int min_z = static_cast<int>(std::floor(center.z - rad_xz));
            int max_z = static_cast<int>(std::floor(center.z + rad_xz));
            int min_y = static_cast<int>(std::floor(center.y - rad_y));
            int max_y = static_cast<int>(std::floor(center.y + rad_y));

            min_x = std::max(min_x, CHUNK_MIN_X);
            max_x = std::min(max_x, CHUNK_MAX_X);
            min_z = std::max(min_z, CHUNK_MIN_Z);
            max_z = std::min(max_z, CHUNK_MAX_Z);
            min_y = std::max(min_y, CHUNK_MIN_Y);
            max_y = std::min(max_y, CHUNK_MAX_Y);

            for (int wx = min_x; wx <= max_x; ++wx) {
                int x = wx - CHUNK_MIN_X;
                for (int wz = min_z; wz <= max_z; ++wz) {
                    int z = wz - CHUNK_MIN_Z;
                    for (int wy = min_y; wy <= max_y; ++wy) {
                        int y = wy;
                        glm::vec3 pos(static_cast<float>(wx),
                                      static_cast<float>(wy),
                                      static_cast<float>(wz));
                        if (point.contains(pos)) {
                            if (y == 0) {
                                continue;
                            }
                            blocks[Chunk::get_index(x, y, z)] = 0;
                        }
                    }
                }
            }
        }
        path.clear_chunk(chunk_pos);
    }
}

Chunk& ChunkGenerator::chunk() { return m_chunk; }

Random& ChunkGenerator::random() { return m_random; }
const std::array<BiomeType, 8>& ChunkGenerator::neighbor_biome() const {
    return m_neighbor_biome;
}
} // namespace Cubed