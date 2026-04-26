#include <Cubed/gameplay/chunk.hpp>
#include <Cubed/gameplay/tree.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/cubed_random.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/math_tools.hpp>
#include <Cubed/tools/perlin_noise.hpp>

#include <numeric>
#include <utility>

namespace Cubed {

Chunk::Chunk(World& world, ChunkPos chunk_pos) : 
    m_chunk_pos(chunk_pos),
    m_world(world)     
{

}

Chunk::~Chunk() {
    if (m_vbo != 0) {
        m_world.push_delete_vbo(m_vbo);
    }
    
}

Chunk::Chunk(Chunk&& other) noexcept : 
    m_dirty(other.is_dirty()),
    m_need_upload(other.m_need_upload.load()),
    m_is_on_gen_vertex_data(other.m_is_on_gen_vertex_data.load()),
    m_vertex_sum(other.m_vertex_sum.load()),
    m_biome(other.m_biome.load()),
    m_chunk_pos(std::move(other.m_chunk_pos)),
    m_world(other.m_world),
    m_heightmap(std::move(other.m_heightmap)),
    m_blocks(std::move(other.m_blocks)),
    m_vbo(other.m_vbo),
    m_vertexs_data(std::move(other.m_vertexs_data))
{
    other.m_vbo = 0;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    //Logger::info("other Chunk pos {} {} in Chunk& Chunk::operator=(Chunk&& other) this {}", other.m_chunk_pos.x, other.m_chunk_pos.z, static_cast<const void*>(&other));
    m_vbo = other.m_vbo;
    other.m_vbo = 0;
    m_chunk_pos = std::move(other.m_chunk_pos);
    m_heightmap = std::move(other.m_heightmap);
    m_blocks = std::move(other.m_blocks);
    m_dirty = other.is_dirty();
    m_vertexs_data = std::move(other.m_vertexs_data);
    m_biome = other.m_biome.load();
    m_is_on_gen_vertex_data = other.m_is_on_gen_vertex_data.load();
    m_need_upload = other.m_need_upload.load();
    m_vertex_sum = other.m_vertex_sum.load();
    return *this;
}

Biome Chunk::get_biome() const {
    return m_biome.load();
}

const std::vector<uint8_t>& Chunk::get_chunk_blocks() const{
    return m_blocks;
}

HeightMapArray Chunk::get_heightmap() const {
    //Logger::info("Chunk pos {} {} in get_heightmap this {}", m_chunk_pos.x, m_chunk_pos.z, static_cast<const void*>(this));
    return m_heightmap;
}

int Chunk::get_index(int x, int y, int z) {
    ASSERT(!(x < 0 || y < 0 || z < 0 || x >= CHUCK_SIZE || y >= WORLD_SIZE_Y || z >= CHUCK_SIZE));
    if ((x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z < 0 || (x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z >= CHUCK_SIZE * CHUCK_SIZE * WORLD_SIZE_Y) {
        Logger::error("block pos x {} y {} z {} range error", x, y, z);
        ASSERT(0);
    }
    return (x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z;
}

int Chunk::get_index(const glm::vec3& pos) {
    return Chunk::get_index(pos.x, pos.y, pos.z);
}

void Chunk::gen_vertex_data(const std::array<const std::vector<uint8_t>*, 4>& neighbor_block) {
    if (m_is_on_gen_vertex_data) {
        return;
    }
    m_is_on_gen_vertex_data = true;
    std::lock_guard lk(m_vertexs_data_mutex);
    m_vertexs_data.clear();
    
    static const glm::ivec3 DIR[6] = {
        {0,0,1},{1,0,0},{0,0,-1},{-1,0,0},{0,1,0},{0,-1,0}
    };

    for (int x = 0; x < SIZE_X; x++) {
        for (int y = 0; y < SIZE_Y; y++) {
            for (int z = 0; z < SIZE_Z; z++) {
                int world_x = x + m_chunk_pos.x * CHUCK_SIZE;
                int world_z = z + m_chunk_pos.z * CHUCK_SIZE;
                int world_y = y;
                int cur_id = m_blocks[get_index(x, y, z)];
                // air
                if (cur_id == 0) {
                    continue;
                }
                
                for (int face = 0; face < 6; face++) {
                    int nx = x + DIR[face].x;
                    int ny = y + DIR[face].y;
                    int nz = z + DIR[face].z;
                    bool neighbor_cull = false;

                    if (nx < 0 || nx >= SIZE_X || ny < 0 || ny >= SIZE_Y || nz < 0 || nz>= SIZE_Z) {
                        
                        int world_nx = world_x + DIR[face].x;
                        int world_ny = world_y + DIR[face].y;
                        int world_nz = world_z + DIR[face].z;

                        auto [neighbor_x, neighbor_z] = World::chunk_pos(world_nx, world_nz);
                        
                        auto is_cull = [&](const std::vector<uint8_t>* chunk_blocks){
                            if (chunk_blocks == nullptr) {
                                return false;
                            }
                            int x, y, z;
                            y = world_ny;
                            x = world_nx - neighbor_x * CHUCK_SIZE;
                            z = world_nz - neighbor_z * CHUCK_SIZE;
                            if (x < 0 || y < 0 || z < 0 || x >= CHUCK_SIZE || y >= WORLD_SIZE_Y || z >= CHUCK_SIZE) {
                                return false;
                            }
                            
                            int idx = Chunk::get_index(x, y, z);
                            // not init
                            if (static_cast<size_t>(idx) >= chunk_blocks->size()) {
                                Logger::warn("not init");
                                return false;
                            }
                            auto id = (*chunk_blocks)[idx];
                            if (is_in_transparent_map(id)) {
                                if (id == cur_id) {
                                    return true;
                                } else {
                                    return false;
                                }
                                
                            } else {
                                return true;
                            }
                        };

                        if (m_chunk_pos.x + 1 == neighbor_x) {
                            neighbor_cull = is_cull(neighbor_block[0]);
                        } else if (m_chunk_pos.x - 1 == neighbor_x) {
                            neighbor_cull = is_cull(neighbor_block[1]);
                        } else if (m_chunk_pos.z + 1 == neighbor_z) {
                            neighbor_cull = is_cull(neighbor_block[2]);
                        } else if (m_chunk_pos.z - 1 == neighbor_z) {
                            neighbor_cull = is_cull(neighbor_block[3]);
                        }
                        //neighbor_cull = m_world.is_block(glm::ivec3(world_x, world_y, world_z) + DIR[face]);
                    } else {
                        auto id = m_blocks[get_index(nx, ny, nz)];
                        if (!is_in_transparent_map(id)) {
                            neighbor_cull = true;
                        } else {
                            if (id == cur_id) {
                                neighbor_cull = true;
                            } else {
                                neighbor_cull = false;
                            }
                        }
                    }

                    if (neighbor_cull) {
                        continue;
                    }
                    for (int i = 0; i < 6; i++) {
                        Vertex vex = {
                            VERTICES_POS[face][i][0] + (float)world_x * 1.0f,
                            VERTICES_POS[face][i][1] + (float)world_y * 1.0f,
                            VERTICES_POS[face][i][2] + (float)world_z * 1.0f,
                            TEX_COORDS[face][i][0],
                            TEX_COORDS[face][i][1],
                            static_cast<float>(cur_id * 6 + face) 

                        };
                        m_vertexs_data.emplace_back(vex);
                    }    
                }
            }
            
        }
    }
    m_vertex_sum = m_vertexs_data.size();
    m_need_upload = true;
    m_is_on_gen_vertex_data = false;
}

GLuint Chunk::get_vbo() const{
    return m_vbo;
}

size_t Chunk::get_vertex_sum() const {
    if (m_vertex_sum == 0) {
        Logger::warn("m_vertex_sum is 0");
    }
    return m_vertex_sum.load();
}


void Chunk::gen_phase_one() {
    float x = static_cast<float>(m_chunk_pos.x);
    float z = static_cast<float>(m_chunk_pos.z);
    float temp  = PerlinNoise::noise(x * BIOME_NOISE_FREQUENCY, 0.0f, z * BIOME_NOISE_FREQUENCY);
    float humid = PerlinNoise::noise(x * BIOME_NOISE_FREQUENCY, 1.0f, z * BIOME_NOISE_FREQUENCY);
    m_biome = get_biome_from_noise(temp, humid);
}

void Chunk::gen_phase_two(const std::array<const Chunk*, 4>& adj_chunks) {
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

void Chunk::gen_phase_three() {
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

void Chunk::gen_phase_four(const std::array<std::optional<HeightMapArray>, 4>& neighbor_heightmap) {
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

void Chunk::gen_phase_five() {
    // bottom
    m_blocks.assign(CHUCK_SIZE * CHUCK_SIZE * WORLD_SIZE_Y, 0);
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < CHUCK_SIZE; z++) {
                m_blocks[get_index(x, y, z)] = 3;
            }
        }
    }

    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {
            int height = static_cast<int>(m_heightmap[x][z]);
            for (int y = 5; y < height - 5; y++) {
                m_blocks[get_index(x, y, z)] = 3;
            }
            if (m_biome == Biome::MOUNTAIN) {
                for (int y = height - 5; y <= height - 1; y++) {
                    if (y > 110) {
                        m_blocks[get_index(x, y, z)] = 3;
                    } else {
                        m_blocks[get_index(x, y, z)] = 2;
                    }
                    
                }
                if (height > 110) {
                    m_blocks[get_index(x, height - 1, z)] = 3;
                } else {
                    m_blocks[get_index(x, height - 1, z)] = 1;
                }
            } else if (m_biome == Biome::DESERT) {
                for (int y = height - 5; y <= height; y++) {
                    m_blocks[get_index(x, y, z)] = 4;
                }
            } else {
                for (int y = height - 5; y <= height - 1; y++) {
                    m_blocks[get_index(x, y, z)] = 2;
                }
                for (int y = height; y <= height; y++) {
                    m_blocks[get_index(x, y, z)] = 1;
                }
            }
        }
    }

}

void Chunk::gen_phase_six(const std::array<std::optional<std::vector<uint8_t>>, 4>& neighbor_block) {
    constexpr int BLEND_RADIUS = 12;
    constexpr int WORLD_HEIGHT = WORLD_SIZE_Y;

    // Helper lambda: get top block type from a neighbor's block data at (nx, nz)
    auto get_top_block_from_neighbor = [&](const std::vector<uint8_t>& blocks, int nx, int nz) -> uint8_t {
        // Search from topmost y downwards for the first non-zero block
        for (int y = WORLD_HEIGHT - 1; y >= 0; --y) {
            int idx = get_index(nx, y, nz); // linear index: y * area + z * size + x
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
            type_self = m_blocks[get_index(x, top_y, z)];

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
                m_blocks[get_index(x, top_y, z)] = final_type;
                unsigned fill_type = 2;
                if (final_type == 1) {
                    fill_type = 2;
                } else if (final_type == 4) {
                    fill_type = 4;
                }
                for (int y = top_y - 5; y < top_y; y++) {
                    m_blocks[get_index(x, y, z)] = fill_type;
                }
            }
        }
    }
}

void Chunk::gen_phase_seven() {
    if (m_biome == Biome::FOREST) {
        std::array<int, SIZE_X> x_arr;
        std::iota(x_arr.begin(), x_arr.end(), 0);
        std::shuffle(x_arr.begin(), x_arr.end(), Cubed::Random::get().engine());
        std::array<int, SIZE_Z> z_arr;
        std::iota(z_arr.begin(), z_arr.end(), 0);
        std::shuffle(z_arr.begin(), z_arr.end(), Cubed::Random::get().engine());
        for (auto x : x_arr) {
            for (auto z : z_arr) {
                if (Cubed::Random::get().random_bool(forest_params().tree_frequency)) {
                    build_tree(*this, {x, static_cast<int>(m_heightmap[x][z]), z});
                }
                
            }
        }
    }

    mark_dirty();
}

void Chunk::upload_to_gpu() {

    ASSERT(is_need_upload());
    if (m_vbo == 0) {
        glGenBuffers(1, &m_vbo);   
    }
    std::lock_guard lk(m_vertexs_data_mutex);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexs_data.size() * sizeof(Vertex), m_vertexs_data.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // after fininshed it, can use
    clear_dirty();
    m_need_upload = false;
}

bool Chunk::is_dirty() const{
    return m_dirty.load();
}

void Chunk::mark_dirty() {
    m_dirty = true;
}

void Chunk::clear_dirty() {
    m_dirty = false;
}

bool Chunk::is_need_upload() const {
    return m_need_upload.load();
}

void Chunk::need_upload() {
    m_need_upload = true;
}

void Chunk::set_chunk_block(int index ,unsigned id) {
    m_blocks[index] = id;
    mark_dirty();
}

}
