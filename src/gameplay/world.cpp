#include <Cubed/gameplay/player.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/cubed_hash.hpp>
World::World() {
    
}

World::~World() {

}

const BlockRenderData& World::get_block_render_data(int world_x, int world_y ,int world_z) {
    int chunk_x, chunk_z;
    if (world_x < 0) {
        chunk_x = (world_x + 1) / CHUCK_SIZE - 1;
    }
    if (world_x >= 0) {
        chunk_x = world_x / CHUCK_SIZE;
    }
    if (world_z < 0) {
        chunk_z = (world_z + 1) / CHUCK_SIZE - 1;
    }
    if (world_z >= 0) {
        chunk_z = world_z / CHUCK_SIZE;
    }
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
    if (m_block_render_data.block_id == 0) {
        return m_block_render_data;
    }
    // draw_face
    m_block_render_data.draw_face.assign(6, true);
    static const std::vector<glm::ivec3> DIR = {
        glm::ivec3(0, 0, 1),
        glm::ivec3(1, 0, 0),
        glm::ivec3(0 ,0, -1),
        glm::ivec3(-1, 0, 0),
        glm::ivec3(0, 1, 0),
        glm::ivec3(0, -1, 0)
    };
    glm::ivec3 world_pos = glm::ivec3(world_x, world_y, world_z);
    for (int i = 0; i < 6; i++) {
        if (is_block(world_pos + DIR[i])) {
            m_block_render_data.draw_face[i] = false;
        }
    }

    return m_block_render_data;
}

const std::optional<LookBlock>& World::get_look_block_pos(const std::string& name) const{
    static std::optional<LookBlock> null_look_block = std::nullopt;
    auto it = m_players.find(HASH::str(name));
    if (it == m_players.end()) {
        LOG::error("Can't find player {}", name);
        CUBED_ASSERT(0);
        return null_look_block;
    }

    return it->second.get_look_block_pos();
    
}

Player& World::get_player(const std::string& name){
    auto it = m_players.find(HASH::str(name));
    if (it == m_players.end()) {
        LOG::error("Can't find player {}", name);
        CUBED_ASSERT(0);
    }
    
    return it->second;
}


void World::init_world() {
    for (int s = 0; s < DISTANCE; s++) {
        for (int t = 0; t < DISTANCE; t++) {
            int ns = s - DISTANCE / 2;
            int nt = t - DISTANCE / 2;

            ChunkPos pos{ns, nt};

            m_chunks.emplace(pos, Chunk(*this, pos)); 
        }
    }
    LOG::info("World init finfish");

    for (auto& chunk_map : m_chunks) {
        auto& [chunk_pos, chunk] = chunk_map;
        chunk.init_chunk();
        
    }
    // After block gen fininshed
    for (auto& chunk_map : m_chunks) {
        auto& [chunk_pos, chunk] = chunk_map;

        chunk.gen_vertex_data();
        
    }
    // init players
    m_players.emplace(HASH::str("TestPlayer"), Player(*this, "TestPlayer"));
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

    }
}

bool World::is_block(const glm::ivec3& block_pos) const{
    int chunk_x, chunk_z;
    int world_x, world_y, world_z;
    world_x = block_pos.x;
    world_y = block_pos.y;
    world_z = block_pos.z;

    if (world_x < 0) {
        chunk_x = (world_x + 1) / CHUCK_SIZE - 1;
    }
    if (world_x >= 0) {
        chunk_x = world_x / CHUCK_SIZE;
    }
    if (world_z < 0) {
        chunk_z = (world_z + 1) / CHUCK_SIZE - 1;
    }
    if (world_z >= 0) {
        chunk_z = world_z / CHUCK_SIZE;
    }

    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return false;
    }

    const auto& chunk_blocks = it->second.get_chunk_blocks();
    int x, y, z;
    y = world_y;
    x = world_x - chunk_x * CHUCK_SIZE;
    z = world_z - chunk_z * CHUCK_SIZE;
    if (x < 0 || y < 0 || z < 0 || x >= CHUCK_SIZE || y >= CHUCK_SIZE || z >= CHUCK_SIZE) {
        return false;
    }
    auto id = chunk_blocks[Chunk::get_index(x, y, z)];
    if (id == 0) {
        return false;
    } else {
        return true;
    }
}

void World::set_block(const glm::ivec3& block_pos, unsigned id) {

    int chunk_x, chunk_z;
    int world_x, world_y, world_z;
    world_x = block_pos.x;
    world_y = block_pos.y;
    world_z = block_pos.z;

    if (world_x < 0) {
        chunk_x = (world_x + 1) / CHUCK_SIZE - 1;
    }
    if (world_x >= 0) {
        chunk_x = world_x / CHUCK_SIZE;
    }
    if (world_z < 0) {
        chunk_z = (world_z + 1) / CHUCK_SIZE - 1;
    }
    if (world_z >= 0) {
        chunk_z = world_z / CHUCK_SIZE;
    }

    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return ;
    }

    int x, y, z;
    y = world_y;
    x = world_x - chunk_x * CHUCK_SIZE;
    z = world_z - chunk_z * CHUCK_SIZE;
    if (x < 0 || y < 0 || z < 0 || x >= CHUCK_SIZE || y >= CHUCK_SIZE || z >= CHUCK_SIZE) {
        return ;
    }

    it->second.set_chunk_block(Chunk::get_index(x, y, z), id);


    int adjacent_chunk_x;
    int adjacent_chunk_z;
    int adj_world_x;
    int adj_world_z;
    if (x == 0) {
        adj_world_x = world_x - 1;
        adj_world_z = world_z;
        if (adj_world_x < 0) {
            adjacent_chunk_x = (adj_world_x + 1) / CHUCK_SIZE - 1;
        }
        if (adj_world_x >= 0) {
            adjacent_chunk_x = adj_world_x / CHUCK_SIZE;
        }
        if (adj_world_z < 0) {
            adjacent_chunk_z = (adj_world_z + 1) / CHUCK_SIZE - 1;
        }
        if (adj_world_z >= 0) {
            adjacent_chunk_z = adj_world_z / CHUCK_SIZE;
        }
        auto adjacent = m_chunks.find(ChunkPos{adjacent_chunk_x, adjacent_chunk_z});
        if (adjacent != m_chunks.end()) {
            adjacent->second.gen_vertex_data();
        }
        
    }

    if (x == CHUCK_SIZE - 1) {
        adj_world_x = world_x + 1;
        adj_world_z = world_z;
        if (adj_world_x < 0) {
            adjacent_chunk_x = (adj_world_x + 1) / CHUCK_SIZE - 1;
        }
        if (adj_world_x >= 0) {
            adjacent_chunk_x = adj_world_x/ CHUCK_SIZE;
        }
        if (adj_world_z < 0) {
            adjacent_chunk_z = (adj_world_z + 1) / CHUCK_SIZE - 1;
        }
        if (adj_world_z >= 0) {
            adjacent_chunk_z = adj_world_z / CHUCK_SIZE;
        }
        auto adjacent = m_chunks.find(ChunkPos{adjacent_chunk_x, adjacent_chunk_z});
        if (adjacent != m_chunks.end()) {
            adjacent->second.gen_vertex_data();
        }
    }

    if (z == 0) {
        adj_world_x = world_x;
        adj_world_z = world_z - 1;
        if (adj_world_x < 0) {
            adjacent_chunk_x = (adj_world_x + 1) / CHUCK_SIZE - 1;
        }
        if (adj_world_x >= 0) {
            adjacent_chunk_x = adj_world_x / CHUCK_SIZE;
        }
        if (adj_world_z < 0) {
            adjacent_chunk_z = (adj_world_z + 1) / CHUCK_SIZE - 1;
        }
        if (adj_world_z >= 0) {
            adjacent_chunk_z = adj_world_z / CHUCK_SIZE;
        }
        
        auto adjacent = m_chunks.find(ChunkPos{adjacent_chunk_x, adjacent_chunk_z});
        if (adjacent != m_chunks.end()) {
            adjacent->second.gen_vertex_data();
        }
    }

    if (z == CHUCK_SIZE - 1) {
        adj_world_x = world_x;
        adj_world_z = world_z + 1;
        if (adj_world_x < 0) {
            adjacent_chunk_x = (adj_world_x + 1) / CHUCK_SIZE - 1;
        }
        if (adj_world_x >= 0) {
            adjacent_chunk_x = adj_world_x/ CHUCK_SIZE;
        }
        if (adj_world_z < 0) {
            adjacent_chunk_z = (adj_world_z + 1) / CHUCK_SIZE - 1;
        }
        if (adj_world_z >= 0) {
            adjacent_chunk_z = adj_world_z / CHUCK_SIZE;
        }
        auto adjacent = m_chunks.find(ChunkPos{adjacent_chunk_x, adjacent_chunk_z});
        if (adjacent != m_chunks.end()) {
            adjacent->second.gen_vertex_data();
        }
    }

}

void World::update(float delta_time) {
    for (auto& player : m_players) {
        player.second.update(delta_time);
    }
}