#include <Cubed/gameplay/world.hpp>
#include <Cubed/tools/cubed_assert.hpp>
World::World() {
    
}

World::~World() {

}
static int chunk_x, chunk_z;
const BlockRenderData& World::get_block_render_data(int world_x, int world_y ,int world_z) {
    
    chunk_x = world_x / CHUCK_SIZE;
    chunk_z = world_z / CHUCK_SIZE;
    //LOG::info("Chunk PosX : {} Chuch PosZ : {}", chunk_x, chunk_z);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});
    CUBED_ASSERT_MSG(it != m_chunks.end(), "Chunk not find");
    
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    int x, y, z;
    y = world_y;
    x = world_x - chunk_x * CHUCK_SIZE;
    z = world_z - chunk_z * CHUCK_SIZE;
    // block id
    m_block_render_data.block_id = chunk_blocks[Chunk::get_index(x, y, z)];
    // draw_face
    m_block_render_data.draw_face.assign(6, true);
    if (x > 0 ) {
        if (x > 0 && chunk_blocks[Chunk::get_index(x - 1, y, z)]) {
            m_block_render_data.draw_face[3] = false;
        }   
    }
    if (x < CHUCK_SIZE - 1) {
        if (x < DISTANCE * CHUCK_SIZE - 1 && chunk_blocks[Chunk::get_index(x + 1, y, z)]) {
            m_block_render_data.draw_face[1] = false;
        }
    }
    if (z > 0 ) {    
        if (z > 0 && chunk_blocks[Chunk::get_index(x, y, z - 1)]) {
            m_block_render_data.draw_face[2] = false;
        }
    }
    if (z < CHUCK_SIZE - 1) {
        if (z < DISTANCE * CHUCK_SIZE - 1 && chunk_blocks[Chunk::get_index(x, y, z + 1)]) {
            m_block_render_data.draw_face[0] = false;
        }
    }
    if (y > 0 ) {
        if (y > 0 && chunk_blocks[Chunk::get_index(x, y - 1, z)]) {
            m_block_render_data.draw_face[5] = false;
        }
    }
    if (y < CHUCK_SIZE - 1) {
        if (y < CHUCK_SIZE - 1 && chunk_blocks[Chunk::get_index(x, y + 1, z)]) {
            m_block_render_data.draw_face[4] = false;
        }
    }
    
    if (x == 0 && world_x - 1 > 0) {
        int adjacent_chunk_x = (world_x - 1) / CHUCK_SIZE;
        int adjacet_chunk_z = world_z / CHUCK_SIZE;
        auto adjacent = m_chunks.find(ChunkPos{adjacent_chunk_x, adjacet_chunk_z});
        if (adjacent != m_chunks.end()) {
            int adjacent_x, adjacent_z;
            const auto& adjacent_chunk_blocks = it->second.get_chunk_blocks();
            int x, y, z;
            y = world_y;
            x = world_x - 1 - adjacent_chunk_x * CHUCK_SIZE;
            z = world_z - adjacet_chunk_z * CHUCK_SIZE;
            if (adjacent_chunk_blocks[Chunk::get_index(x, y, z)]) {
                m_block_render_data.draw_face[3] = false;
            }
        }
        
    }

    if (x == CHUCK_SIZE - 1) {
        int adjacent_chunk_x = (world_x + 1) / CHUCK_SIZE;
        int adjacet_chunk_z = world_z / CHUCK_SIZE;
        auto adjacent = m_chunks.find(ChunkPos{adjacent_chunk_x, adjacet_chunk_z});
        if (adjacent != m_chunks.end()) {
            int adjacent_x, adjacent_z;
            const auto& adjacent_chunk_blocks = it->second.get_chunk_blocks();
            int x, y, z;
            y = world_y;
            x = world_x + 1 - adjacent_chunk_x * CHUCK_SIZE;
            z = world_z - adjacet_chunk_z * CHUCK_SIZE;
            if (adjacent_chunk_blocks[Chunk::get_index(x, y, z)]) {
                m_block_render_data.draw_face[1] = false;
            }
        }
    }

    if (z == 0 && world_z - 1 > 0) {
        int adjacent_chunk_x = world_x / CHUCK_SIZE;
        int adjacet_chunk_z = (world_z - 1) / CHUCK_SIZE;
        
        auto adjacent = m_chunks.find(ChunkPos{adjacent_chunk_x, adjacet_chunk_z});
        if (adjacent != m_chunks.end()) {
            int adjacent_x, adjacent_z;
            const auto& adjacent_chunk_blocks = it->second.get_chunk_blocks();
            int x, y, z;
            y = world_y;
            x = world_x - adjacent_chunk_x * CHUCK_SIZE;
            z = world_z - 1 - adjacet_chunk_z * CHUCK_SIZE;
            if (adjacent_chunk_blocks[Chunk::get_index(x, y, z)]) {
                m_block_render_data.draw_face[2] = false;
            }
        }
    }

    if (z == CHUCK_SIZE - 1) {
        int adjacent_chunk_x = world_x / CHUCK_SIZE;
        int adjacet_chunk_z = (world_z + 1) / CHUCK_SIZE;
        auto adjacent = m_chunks.find(ChunkPos{adjacent_chunk_x, adjacet_chunk_z});
        if (adjacent != m_chunks.end()) {
            int adjacent_x, adjacent_z;
            const auto& adjacent_chunk_blocks = it->second.get_chunk_blocks();
            int x, y, z;
            y = world_y;
            x = world_x - adjacent_chunk_x * CHUCK_SIZE;
            z = world_z + 1 - adjacet_chunk_z * CHUCK_SIZE;
            if (adjacent_chunk_blocks[Chunk::get_index(x, y, z)]) {
                m_block_render_data.draw_face[0] = false;
            }
        }
    }

    return m_block_render_data;
}


void World::init_world() {
    for (int s = 0; s < DISTANCE; s++) {
        for (int t = 0; t < DISTANCE; t++) {
            ChunkPos pos{s, t};
            LOG::info("Chunk Pos in init_world X: {} Z: {}", pos.x, pos.z);
            m_chunks.emplace(pos, Chunk(*this, pos)); 
        }
    }
    LOG::info("World init finfish");

    for (auto& chunk_map : m_chunks) {
        auto& [chunk_pos, chunk] = chunk_map;
        chunk.init_chunk();
    }

}

void World::render() {
    for (const auto& chunk_map : m_chunks) {
        const auto& [pos, chunk] = chunk_map;
        
        glBindBuffer(GL_ARRAY_BUFFER, chunk.get_vbo());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, s));
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, layer));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glDrawArrays(GL_TRIANGLES, 0, chunk.get_vertex_data().size() * 3);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        //LOG::info("Chunk {} {} render finished", pos.x, pos.z);
    }
}