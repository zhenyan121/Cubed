#include <Cubed/gameplay/chunk.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/perlin_noise.hpp>
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
    m_vertexs_data(std::move(other.m_vertexs_data))
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
    return *this;
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
// this is thread-unsafe!
void Chunk::gen_vertex_data() {
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
                int id = m_blocks[get_index(x, y, z)];
                // air
                if (id == 0) {
                    continue;
                }
                for (int face = 0; face < 6; face++) {
                    int nx = x + DIR[face].x;
                    int ny = y + DIR[face].y;
                    int nz = z + DIR[face].z;
                    bool neighbor_soild = false;

                    if (nx < 0 || nx >= SIZE_X || ny < 0 || ny >= SIZE_Y || nz < 0 || nz>= SIZE_Z) {
                        neighbor_soild = m_world.is_block(glm::ivec3(world_x, world_y, world_z) + DIR[face]);
                    } else {
                        if (m_blocks[get_index(nx, ny, nz)] != 0) {
                            neighbor_soild = true;
                        }
                    }

                    if (neighbor_soild) {
                        continue;
                    }
                    for (int i = 0; i < 6; i++) {
                        Vertex vex = {
                            VERTICES_POS[face][i][0] + (float)world_x * 1.0f,
                            VERTICES_POS[face][i][1] + (float)world_y * 1.0f,
                            VERTICES_POS[face][i][2] + (float)world_z * 1.0f,
                            TEX_COORDS[face][i][0],
                            TEX_COORDS[face][i][1],
                            static_cast<float>(id * 6 + face) 

                        };
                        m_vertexs_data.emplace_back(vex);
                    }    
                }
            }
            
        }
    }
    
}

void Chunk::gen_vertex_data(const std::vector<const std::vector<uint8_t>*>& neighbor_block) {
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
                int id = m_blocks[get_index(x, y, z)];
                // air
                if (id == 0) {
                    continue;
                }
                for (int face = 0; face < 6; face++) {
                    int nx = x + DIR[face].x;
                    int ny = y + DIR[face].y;
                    int nz = z + DIR[face].z;
                    bool neighbor_soild = false;

                    if (nx < 0 || nx >= SIZE_X || ny < 0 || ny >= SIZE_Y || nz < 0 || nz>= SIZE_Z) {
                        
                        int world_nx = world_x + DIR[face].x;
                        int world_ny = world_y + DIR[face].y;
                        int world_nz = world_z + DIR[face].z;

                        auto [neighbor_x, neighbor_z] = World::chunk_pos(world_nx, world_nz);
                        
                        auto is_block = [&](const std::vector<uint8_t>* chunk_blocks){
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
                            if (id == 0) {
                                return false;
                            } else {
                                return true;
                            }
                        };

                        if (m_chunk_pos.x + 1 == neighbor_x) {
                            neighbor_soild = is_block(neighbor_block[0]);
                        } else if (m_chunk_pos.x - 1 == neighbor_x) {
                            neighbor_soild = is_block(neighbor_block[1]);
                        } else if (m_chunk_pos.z + 1 == neighbor_z) {
                            neighbor_soild = is_block(neighbor_block[2]);
                        } else if (m_chunk_pos.z - 1 == neighbor_z) {
                            neighbor_soild = is_block(neighbor_block[3]);
                        }
                        //neighbor_soild = m_world.is_block(glm::ivec3(world_x, world_y, world_z) + DIR[face]);
                    } else {
                        if (m_blocks[get_index(nx, ny, nz)] != 0) {
                            neighbor_soild = true;
                        }
                    }

                    if (neighbor_soild) {
                        continue;
                    }
                    for (int i = 0; i < 6; i++) {
                        Vertex vex = {
                            VERTICES_POS[face][i][0] + (float)world_x * 1.0f,
                            VERTICES_POS[face][i][1] + (float)world_y * 1.0f,
                            VERTICES_POS[face][i][2] + (float)world_z * 1.0f,
                            TEX_COORDS[face][i][0],
                            TEX_COORDS[face][i][1],
                            static_cast<float>(id * 6 + face) 

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
    m_blocks.assign(CHUCK_SIZE * CHUCK_SIZE * WORLD_SIZE_Y, 0);
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < CHUCK_SIZE; z++) {
                m_blocks[get_index(x, y, z)] = 1;
            }
        }
    }

    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {
             
            float world_x = static_cast<float>(x + m_chunk_pos.x * CHUCK_SIZE);
            float world_z = static_cast<float>(z + m_chunk_pos.z * CHUCK_SIZE);
            
            float noise =
                0.5f * PerlinNoise::noise(world_x * 0.01f, world_z * 0.01f, 0.5f) +
                0.25f * PerlinNoise::noise(world_x * 0.02f, world_z * 0.02f, 0.5f) +
                0.125f * PerlinNoise::noise(world_x * 0.04f, world_z * 0.04f, 0.5f);
            int y_max = height * noise;

            for (int y = 5; y < y_max; y++) {
                m_blocks[get_index(x, y, z)] = 1;
            }

        }
    }

    mark_dirty();
    
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


