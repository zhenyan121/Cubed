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
    glDeleteBuffers(1, &m_vbo);
}

const std::vector<uint8_t>& Chunk::get_chunk_blocks() const{
    return m_blocks;
}


int Chunk::get_index(int x, int y, int z) {
    if ((x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z < 0 || (x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z >= CHUCK_SIZE * CHUCK_SIZE * WORLD_SIZE_Y) {
        Logger::error("block pos x {} y {} z {} range error", x, y, z);
        CUBED_ASSERT(0);
    }
    return (x * WORLD_SIZE_Y + y) * CHUCK_SIZE + z;
}

void Chunk::gen_vertex_data() {
    m_vertexs_data.clear();
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE_Y; y++) {
            for (int z = 0; z < CHUCK_SIZE; z++) {
                int world_x = x + m_chunk_pos.x * CHUCK_SIZE;
                int world_z = z + m_chunk_pos.z * CHUCK_SIZE;
                int world_y = y;
                const auto& block_render_data = m_world.get_block_render_data(world_x, world_y, world_z);
                // air
                if (m_blocks[get_index(x, y, z)] == 0) {
                    continue;
                }
                for (int face = 0; face < 6; face++) {
                    if (!block_render_data.draw_face[face]) {
                        continue;
                    }
                    for (int i = 0; i < 6; i++) {
                        Vertex vex = {
                            VERTICES_POS[face][i][0] + (float)world_x * 1.0f,
                            VERTICES_POS[face][i][1] + (float)world_y * 1.0f,
                            VERTICES_POS[face][i][2] + (float)world_z * 1.0f,
                            TEX_COORDS[face][i][0],
                            TEX_COORDS[face][i][1],
                            static_cast<float>(block_render_data.block_id * 6 + face) 

                        };
                        m_vertexs_data.emplace_back(vex);
                    }    
                }
            }
            
        }
    }

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexs_data.size() * sizeof(Vertex), m_vertexs_data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    

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

    m_is_gened = true;
    
}

bool Chunk::is_dirty() const{
    return m_dirty;
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


