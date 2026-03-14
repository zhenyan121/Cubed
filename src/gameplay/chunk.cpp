#include <Cubed/gameplay/chunk.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/tools/log.hpp>
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
    return x * CHUCK_SIZE * CHUCK_SIZE + y * CHUCK_SIZE + z;
}

void Chunk::gen_vertex_data() {
    m_vertexs_data.clear();
    glDeleteBuffers(1, &m_vbo);
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {
            for (int y = 0; y < CHUCK_SIZE; y++) {
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
    m_blocks.assign(CHUCK_SIZE * CHUCK_SIZE * CHUCK_SIZE, 0);
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < CHUCK_SIZE; z++) {
                m_blocks[get_index(x, y, z)] = 1;
            }
        }
    }
    
}

void Chunk::set_chunk_block(int index ,unsigned id) {
    m_blocks[index] = id;
    glDeleteBuffers(1, &m_vbo);
    
    gen_vertex_data();
}


