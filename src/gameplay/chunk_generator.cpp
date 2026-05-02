#include "Cubed/gameplay/chunk_generator.hpp"

#include "Cubed/gameplay/builders/desert_builder.hpp"
#include "Cubed/gameplay/builders/forest_builder.hpp"
#include "Cubed/gameplay/builders/mountain_builder.hpp"
#include "Cubed/gameplay/builders/plain_builder.hpp"
#include "Cubed/gameplay/builders/river_builder.hpp"
#include "Cubed/gameplay/chunk.hpp"
#include "Cubed/gameplay/tree.hpp"
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
    const std::array<const Chunk*, 4>& adj_chunks) {
    auto m_biome = m_chunk.biome();
    for (int i = 0; i < 4; i++) {
        auto& chunk = adj_chunks[i];
        if (chunk == nullptr) {
            continue;
        }
        BiomeType biome = chunk->get_biome();
        neighbor_biome[i] = biome;
        if (biome == BiomeType::RIVER) {
            is_neighbor_river = true;
        }
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

    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {

            float world_x = static_cast<float>(x + m_chunk_pos.x * CHUCK_SIZE);
            float world_z = static_cast<float>(z + m_chunk_pos.z * CHUCK_SIZE);

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
    const std::array<std::optional<HeightMapArray>, 4>& neighbor_heightmap) {
    auto& m_heightmap = m_chunk.heightmap();
    auto m_biome = m_chunk.biome();

    // --- Right neighbor neighbor[0]: (1, 0) ---
    for (int z = 0; z < SIZE_Z; z++) {
        if (neighbor_heightmap[0] != std::nullopt &&
            neighbor_biome[0] != m_biome) {
            int edge_x = CHUCK_SIZE - 1;
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
            int edge_x = 0;
            int h = m_heightmap[edge_x][z];
            int neighbor_h = (*neighbor_heightmap[1])[CHUCK_SIZE - 1][z];
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
            int edge_z = CHUCK_SIZE - 1;
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
            int edge_z = 0;
            int h = m_heightmap[x][edge_z];
            int neighbor_h = (*neighbor_heightmap[3])[x][CHUCK_SIZE - 1];
            if (h <= neighbor_h) {
                continue;
            }
            int delta_h = h - neighbor_h;
            int step = delta_h / BLEND_RADIUS;
            if (step < 1) {
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
}

void ChunkGenerator::generate_terrain_blocks() {
    make_biome_builder();
    if (!m_biome_builder) {
        Logger::error("BiomeBuilder is nullptr");
        return;
    }
    m_chunk.blocks().assign(CHUCK_SIZE * CHUCK_SIZE * WORLD_SIZE_Y, 0);
    m_biome_builder->build_biome();
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
    for (int x = 0; x < CHUCK_SIZE; ++x) {
        for (int z = 0; z < CHUCK_SIZE; ++z) {
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
            if (neighbor_block[0] && x >= CHUCK_SIZE - BLEND_RADIUS) {
                int dist = (CHUCK_SIZE - 1) - x;
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
                        *neighbor_block[1], CHUCK_SIZE - 1, z);
                    weights[type_neighbor] += t;
                }
            }

            // --- Front neighbor (index 2) ---
            if (neighbor_block[2] && z >= CHUCK_SIZE - BLEND_RADIUS) {
                int dist = (CHUCK_SIZE - 1) - z;
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
                        *neighbor_block[3], x, CHUCK_SIZE - 1);
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
                if (is_neighbor_river && final_type == 1) {
                    if (top_y < SEA_LEVEL) {
                        final_type = 2;
                    } else {
                        final_type = 1;
                    }
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

Chunk& ChunkGenerator::chunk() { return m_chunk; }

Random& ChunkGenerator::random() { return m_random; }
bool ChunkGenerator::neighbor_river() const { return is_neighbor_river; }

} // namespace Cubed