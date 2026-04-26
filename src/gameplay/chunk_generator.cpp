#include <Cubed/gameplay/chunk_generator.hpp>

#include <Cubed/gameplay/chunk.hpp>
#include <Cubed/gameplay/tree.hpp>
#include <Cubed/tools/cubed_hash.hpp>
#include <Cubed/tools/perlin_noise.hpp>
namespace Cubed {

ChunkGenerator::ChunkGenerator(Chunk& chunk) :
    m_chunk(chunk)
{
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

const unsigned& ChunkGenerator::seed() {
    return m_generator_seed;
}
void ChunkGenerator::seed(unsigned s) {
    is_seed_change = true;
    m_generator_seed = s;
}


void ChunkGenerator::assign_chunk_biome() {
    auto m_chunk_pos = m_chunk.chunk_pos();
    float x = static_cast<float>(m_chunk_pos.x);
    float z = static_cast<float>(m_chunk_pos.z);
    float temp  = PerlinNoise::noise(x * BIOME_NOISE_FREQUENCY, 0.0f, z * BIOME_NOISE_FREQUENCY);
    float humid = PerlinNoise::noise(x * BIOME_NOISE_FREQUENCY, 1.0f, z * BIOME_NOISE_FREQUENCY);
    auto biome = get_biome_from_noise(temp, humid);
    m_chunk.biome(biome);
}

void ChunkGenerator::resolve_biome_adjacency_conflict(const std::array<const Chunk*, 4>& adj_chunks) {
    auto m_biome = m_chunk.biome();
    for (auto& chunk : adj_chunks) {
        if (chunk == nullptr) {
            continue;
        }
        Biome biome = chunk->get_biome();
        for (const auto& non : NON_ADJACENT) {
            if (m_biome != non.first) {
                continue;
            }
            for (auto b : non.second) {
                if (b == biome) {
                    m_biome = non.replace;
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

            auto sample_height = [&](Biome b) -> float {
                auto range = get_biome_height_range(b);
                auto [f1, f2, f3] = get_noise_frequencies_for_biome(b);
                float n =
                    1.00f * PerlinNoise::noise(world_x * f1, 0.5f, world_z * f1) +
                    0.50f * PerlinNoise::noise(world_x * f2, 0.5f, world_z * f2) +
                    0.25f * PerlinNoise::noise(world_x * f3, 0.5f, world_z * f3);
                n /= 1.75f;
                return range.base_y + n * range.amplitude;
            };
            m_heightmap[x][z] = sample_height(m_biome); 
        }
    }
}

void ChunkGenerator::blend_heightmap_boundaries(const std::array<std::optional<HeightMapArray>, 4>& neighbor_heightmap) {
    auto& m_heightmap = m_chunk.heightmap();

    // Width of interpolation influence (in number of cells)
    constexpr int BLEND_RADIUS = 12;

    for (int x = 0; x < SIZE_X; x++) {
        for (int z = 0; z < SIZE_Z; z++) {
            float h = static_cast<float>(m_heightmap[x][z]);
            float total_weight = 1.0f;
            float blended = h;

            // --- Right neighbor neighbor[0]: (1, 0) ---
            // Blend when x is close to SIZE_X-1
            if (neighbor_heightmap[0] != std::nullopt) {
                int dist = (SIZE_X - 1) - x; // distance from right border
                if (dist < BLEND_RADIUS) {
                    // Neighbor's boundary row is its x=0 column
                    float neighbor_h = static_cast<float>((*neighbor_heightmap[0])[0][z]);
                    float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS; // larger weight when closer
                    // Use smoothstep for a more natural transition
                    t = t * t * (3.0f - 2.0f * t);
                    blended += t * neighbor_h;
                    total_weight += t;
                }
            }

            // --- Left neighbor neighbor[1]: (-1, 0) ---
            if (neighbor_heightmap[1] != std::nullopt) {
                int dist = x; // distance from left border
                if (dist < BLEND_RADIUS) {
                    float neighbor_h = static_cast<float>((*neighbor_heightmap[1])[SIZE_X - 1][z]);
                    float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                    t = t * t * (3.0f - 2.0f * t);
                    blended += t * neighbor_h;
                    total_weight += t;
                }
            }

            // --- Front neighbor neighbor[2]: (0, 1) ---
            if (neighbor_heightmap[2] != std::nullopt) {
                int dist = (SIZE_Z - 1) - z;
                if (dist < BLEND_RADIUS) {
                    float neighbor_h = static_cast<float>((*neighbor_heightmap[2])[x][0]);
                    float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                    t = t * t * (3.0f - 2.0f * t);
                    blended += t * neighbor_h;
                    total_weight += t;
                }
            }

            // --- Back neighbor neighbor[3]: (0, -1) ---
            if (neighbor_heightmap[3] != std::nullopt) {
                int dist = z;
                if (dist < BLEND_RADIUS) {
                    float neighbor_h = static_cast<float>((*neighbor_heightmap[3])[x][SIZE_Z - 1]);
                    float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                    t = t * t * (3.0f - 2.0f * t);
                    blended += t * neighbor_h;
                    total_weight += t;
                }
            }

            m_heightmap[x][z] = static_cast<int>(blended / total_weight);
        }
    }
}

void ChunkGenerator::generate_terrain_blocks() {
    auto& m_blocks = m_chunk.blocks();
    auto& m_heightmap = m_chunk.heightmap();
    auto m_biome = m_chunk.biome();
    // bottom
    m_blocks.assign(CHUCK_SIZE * CHUCK_SIZE * WORLD_SIZE_Y, 0);
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < CHUCK_SIZE; z++) {
                m_blocks[Chunk::get_index(x, y, z)] = 3;
            }
        }
    }

    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {
            int height = static_cast<int>(m_heightmap[x][z]);
            for (int y = 5; y < height - 5; y++) {
                m_blocks[Chunk::get_index(x, y, z)] = 3;
            }
            if (m_biome == Biome::MOUNTAIN) {
                for (int y = height - 5; y <= height - 1; y++) {
                    if (y > 110) {
                        m_blocks[Chunk::get_index(x, y, z)] = 3;
                    } else {
                        m_blocks[Chunk::get_index(x, y, z)] = 2;
                    }
                    
                }
                if (height > 110) {
                    m_blocks[Chunk::get_index(x, height - 1, z)] = 3;
                } else {
                    m_blocks[Chunk::get_index(x, height - 1, z)] = 1;
                }
            } else if (m_biome == Biome::DESERT) {
                for (int y = height - 5; y <= height; y++) {
                    m_blocks[Chunk::get_index(x, y, z)] = 4;
                }
            } else {
                for (int y = height - 5; y <= height - 1; y++) {
                    m_blocks[Chunk::get_index(x, y, z)] = 2;
                }
                for (int y = height; y <= height; y++) {
                    m_blocks[Chunk::get_index(x, y, z)] = 1;
                }
            }
        }
    }
}

void ChunkGenerator::blend_surface_blocks_borders(const std::array<std::optional<std::vector<uint8_t>>, 4>& neighbor_block) {
    auto& m_blocks = m_chunk.blocks();
    auto& m_heightmap = m_chunk.heightmap();
    
    constexpr int BLEND_RADIUS = 12;
    constexpr int WORLD_HEIGHT = WORLD_SIZE_Y;

    // Helper lambda: get top block type from a neighbor's block data at (nx, nz)
    auto get_top_block_from_neighbor = [&](const std::vector<uint8_t>& blocks, int nx, int nz) -> uint8_t {
        // Search from topmost y downwards for the first non-zero block
        for (int y = WORLD_HEIGHT - 1; y >= 0; --y) {
            int idx = Chunk::get_index(nx, y, nz); // linear index: y * area + z * size + x
            if (idx >= 0 && idx < static_cast<int>(blocks.size()) && blocks[idx] != 0) {
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

            if (top_y == -1) continue; // no block? skip

            // Weight map: type -> total weight
            std::unordered_map<uint8_t, float> weights;
            weights[type_self] = 1.0f; // self weight

            // --- Right neighbor (index 0) ---
            if (neighbor_block[0] && x >= CHUCK_SIZE - BLEND_RADIUS) {
                int dist = (CHUCK_SIZE - 1) - x;
                float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                t = t * t * (3.0f - 2.0f * t); // smoothstep
                if (t > 0.0f) {
                    uint8_t type_neighbor = get_top_block_from_neighbor(*neighbor_block[0], 0, z);
                    weights[type_neighbor] += t;
                }
            }

            // --- Left neighbor (index 1) ---
            if (neighbor_block[1] && x < BLEND_RADIUS) {
                int dist = x;
                float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                t = t * t * (3.0f - 2.0f * t);
                if (t > 0.0f) {
                    uint8_t type_neighbor = get_top_block_from_neighbor(*neighbor_block[1], CHUCK_SIZE - 1, z);
                    weights[type_neighbor] += t;
                }
            }

            // --- Front neighbor (index 2) ---
            if (neighbor_block[2] && z >= CHUCK_SIZE - BLEND_RADIUS) {
                int dist = (CHUCK_SIZE - 1) - z;
                float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                t = t * t * (3.0f - 2.0f * t);
                if (t > 0.0f) {
                    uint8_t type_neighbor = get_top_block_from_neighbor(*neighbor_block[2], x, 0);
                    weights[type_neighbor] += t;
                }
            }

            // --- Back neighbor (index 3) ---
            if (neighbor_block[3] && z < BLEND_RADIUS) {
                int dist = z;
                float t = 1.0f - static_cast<float>(dist) / BLEND_RADIUS;
                t = t * t * (3.0f - 2.0f * t);
                if (t > 0.0f) {
                    uint8_t type_neighbor = get_top_block_from_neighbor(*neighbor_block[3], x, CHUCK_SIZE - 1);
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
                m_blocks[Chunk::get_index(x, top_y, z)] = final_type;
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
    auto m_biome = m_chunk.biome();
    auto& m_heightmap = m_chunk.heightmap();
    if (m_biome == Biome::FOREST) {
        std::array<int, SIZE_X> x_arr;
        std::iota(x_arr.begin(), x_arr.end(), 0);
        std::shuffle(x_arr.begin(), x_arr.end(), m_random.engine());
        std::array<int, SIZE_Z> z_arr;
        std::iota(z_arr.begin(), z_arr.end(), 0);
        std::shuffle(z_arr.begin(), z_arr.end(), m_random.engine());
        for (auto x : x_arr) {
            for (auto z : z_arr) {
                if (m_random.random_bool(forest_params().tree_frequency)) {
                    build_tree(m_chunk, {x, static_cast<int>(m_heightmap[x][z]), z});
                }
                
            }
        }
    }
}



}