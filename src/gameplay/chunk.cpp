#include <Cubed/gameplay/chunk.hpp>
#include <Cubed/gameplay/tree.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/cubed_random.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/math_tools.hpp>
#include <Cubed/tools/perlin_noise.hpp>

#include <utility>
Chunk::Chunk(World& world, ChunkPos chunk_pos) : 
    m_world(world),
    m_chunk_pos(chunk_pos)     
{

}

Chunk::~Chunk() {
    if (m_vbo != 0) {
        m_world.push_delete_vbo(m_vbo);
    }
    
}

Chunk::Chunk(Chunk&& other) : 
    m_vbo(other.m_vbo),
    m_chunk_pos(std::move(other.m_chunk_pos)),
    m_world(other.m_world),
    m_blocks(std::move(other.m_blocks)),
    m_dirty(other.is_dirty()),
    m_vertexs_data(std::move(other.m_vertexs_data)),
    m_biome(other.m_biome)
{
    other.m_vbo = 0;
}

Chunk& Chunk::operator=(Chunk&& other) {
    m_vbo = other.m_vbo;
    other.m_vbo = 0;
    m_chunk_pos = std::move(other.m_chunk_pos);
    m_blocks = std::move(other.m_blocks);
    m_dirty = other.is_dirty();
    m_vertexs_data = std::move(other.m_vertexs_data);
    m_biome = other.m_biome;
    return *this;
}

Biome Chunk::get_biome() const {
    return m_biome;
}

const std::vector<uint8_t>& Chunk::get_chunk_blocks() const{
    return m_blocks;
}

int Chunk::get_index(int x, int y, int z) {
    CUBED_ASSERT(!(x < 0 || y < 0 || z < 0 || x >= CHUCK_SIZE || y >= WORLD_SIZE_Y || z >= CHUCK_SIZE));
    if ((x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z < 0 || (x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z >= CHUCK_SIZE * CHUCK_SIZE * WORLD_SIZE_Y) {
        Logger::error("block pos x {} y {} z {} range error", x, y, z);
        CUBED_ASSERT(0);
    }
    return (x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z;
}

int Chunk::get_index(const glm::vec3& pos) {
    return Chunk::get_index(pos.x, pos.y, pos.z);
}

void Chunk::gen_vertex_data(const std::array<const std::vector<uint8_t>*, 4>& neighbor_block) {
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
                            if (idx >= chunk_blocks->size()) {
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
    
}

GLuint Chunk::get_vbo() const{
    return m_vbo;
}

const std::vector<Vertex>& Chunk::get_vertex_data() const{
    return m_vertexs_data;
}

void Chunk::init_chunk() {
    resolve_biome();
    resolve_blocks();
}

void Chunk::upload_to_gpu() {

    CUBED_ASSERT(is_dirty());
    if (m_vbo == 0) {
        glGenBuffers(1, &m_vbo);   
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexs_data.size() * sizeof(Vertex), m_vertexs_data.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // after fininshed it, can use
    clear_dirty();

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

void Chunk::set_chunk_block(int index ,unsigned id) {
    m_blocks[index] = id;
    mark_dirty();
}


void Chunk::resolve_biome() {
    float cx = (m_chunk_pos.x + 0.5f) * CHUCK_SIZE;
    float cz = (m_chunk_pos.z + 0.5f) * CHUCK_SIZE;
    float temp  = PerlinNoise::noise(cx * BIOME_NOISE_FREQUENCY, 0.0f, cz * BIOME_NOISE_FREQUENCY);
    float humid = PerlinNoise::noise(cx * BIOME_NOISE_FREQUENCY, 1.0f, cz * BIOME_NOISE_FREQUENCY);
    m_biome = get_biome_from_noise(temp, humid);
}

void Chunk::resolve_blocks() {
    m_blocks.assign(CHUCK_SIZE * CHUCK_SIZE * WORLD_SIZE_Y, 0);
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < CHUCK_SIZE; z++) {
                m_blocks[get_index(x, y, z)] = 3;
            }
        }
    }
    std::array<std::array<int, SIZE_Z>, SIZE_X> heights;
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {
             
            float world_x = static_cast<float>(x + m_chunk_pos.x * CHUCK_SIZE);
            float world_z = static_cast<float>(z + m_chunk_pos.z * CHUCK_SIZE);

            float temp  = PerlinNoise::noise(world_x * BIOME_NOISE_FREQUENCY, 0.0f, world_z * BIOME_NOISE_FREQUENCY);
            float humid = PerlinNoise::noise(world_x * BIOME_NOISE_FREQUENCY, 1.0f, world_z * BIOME_NOISE_FREQUENCY);
            int height = get_interpolated_height(world_x, world_z, temp, humid);
            heights[x][z] = height;
            auto biome = get_biome_from_noise(temp, humid);
            for (int y = 5; y < height - 5; y++) {
                m_blocks[get_index(x, y, z)] = 3;
            }
            if (biome == Biome::MOUNTAIN) {
                for (int y = height - 5; y <= height - 1; y++) {
                    if (y > 101) {
                        m_blocks[get_index(x, y, z)] = 3;
                    } else {
                        m_blocks[get_index(x, y, z)] = 2;
                    }
                    
                }
                if (height > 101) {
                    m_blocks[get_index(x, height - 1, z)] = 3;
                } else {
                    m_blocks[get_index(x, height - 1, z)] = 1;
                }
            } else if (biome == Biome::DESERT) {
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
    if (m_biome == Biome::FOREST) {
        std::array<int, SIZE_X> x_arr;
        std::ranges::iota(x_arr, 0);
        std::shuffle(x_arr.begin(), x_arr.end(), Cubed::Random::get().engine());
        std::array<int, SIZE_Z> z_arr;
        std::ranges::iota(z_arr, 0);
        std::shuffle(z_arr.begin(), z_arr.end(), Cubed::Random::get().engine());
        for (auto x : x_arr) {
            for (auto z : z_arr) {
                if (Cubed::Random::get().random_bool(0.8)) {
                    build_tree(*this, {x, heights[x][z], z});
                }
                
            }
        }
    }

    mark_dirty();
}

