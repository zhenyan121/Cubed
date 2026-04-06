#include <Cubed/gameplay/player.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/cubed_hash.hpp>
#include <Cubed/tools/math_tools.hpp>

#include <unordered_set>

World::World() {
    
}

World::~World() {

}

bool World::can_move(const AABB& player_box) const{
    
    

    return true;
}

const BlockRenderData& World::get_block_render_data(int world_x, int world_y ,int world_z) {
    auto [chunk_x, chunk_z] = chunk_pos(world_x, world_z);
    //Logger::info("Chunk PosX : {} Chuch PosZ : {}", chunk_x, chunk_z);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});
    CUBED_ASSERT_MSG(it != m_chunks.end(), "Chunk not find");
    
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    int x, y, z;
    y = world_y;
    x = world_x - chunk_x * CHUCK_SIZE;
    z = world_z - chunk_z * CHUCK_SIZE;
    //BlockRenderData m_block_render_data;
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
        Logger::error("Can't find player {}", name);
        CUBED_ASSERT(0);
        return null_look_block;
    }

    return it->second.get_look_block_pos();
    
}

Player& World::get_player(const std::string& name){
    auto it = m_players.find(HASH::str(name));
    if (it == m_players.end()) {
        Logger::error("Can't find player {}", name);
        CUBED_ASSERT(0);
    }
    
    return it->second;
}


void World::init_world() {
    auto t1 = std::chrono::system_clock::now();
    for (int s = 0; s < DISTANCE; s++) {
        for (int t = 0; t < DISTANCE; t++) {
            int ns = s - DISTANCE / 2;
            int nt = t - DISTANCE / 2;

            ChunkPos pos{ns, nt};

            m_chunks.emplace(pos, Chunk(*this, pos)); 
        }
    }

    for (auto& chunk_map : m_chunks) {
        auto& [chunk_pos, chunk] = chunk_map;
        chunk.init_chunk();
        
    }
    // After block gen fininshed
    for (auto& chunk_map : m_chunks) {
        auto& [chunk_pos, chunk] = chunk_map;

        chunk.gen_vertex_data();
        
    }
    auto t2 = std::chrono::system_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    Logger::info("Chunk Block Init Finish, Time Consuming: {}", d);
    // init players
    m_players.emplace(HASH::str("TestPlayer"), Player(*this, "TestPlayer"));
    Logger::info("TestPlayer Create Finish");
}

void World::render(const glm::mat4& mvp_matrix) {
    Math::extract_frustum_planes(mvp_matrix, m_planes);
    for (const auto& chunk_map : m_chunks) {
        const auto& [pos, chunk] = chunk_map;
        glm::vec3 center = glm::vec3(static_cast<float>(pos.x * CHUCK_SIZE) + static_cast<float>(CHUCK_SIZE / 2), static_cast<float>(WORLD_SIZE_Y/ 2), static_cast<float>(pos.z * CHUCK_SIZE) + static_cast<float>(CHUCK_SIZE / 2));
        if (is_aabb_in_frustum(center, glm::vec3(static_cast<float>(CHUCK_SIZE / 2), static_cast<float>(WORLD_SIZE_Y / 2), static_cast<float>(CHUCK_SIZE / 2)))) {
            glBindBuffer(GL_ARRAY_BUFFER, chunk.get_vbo());
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, s));
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, layer));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);

            glDrawArrays(GL_TRIANGLES, 0, chunk.get_vertex_data().size());
            glBindBuffer(GL_ARRAY_BUFFER, 0);

        }
        

    }

}

std::pair<int, int> World::chunk_pos(int world_x, int world_z) const{
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
    return {chunk_x, chunk_z};
}

void World::gen_chunks() {
    const auto& player =  get_player("TestPlayer");
    const auto& player_pos = player.get_player_pos();

    int x = std::floor(player_pos.x);
    int z = std::floor(player_pos.z);
    auto [chunk_x, chunk_z] = chunk_pos(x, z);
    std::unordered_set<ChunkPos, ChunkPos::Hash> cur_chunks;
    std::vector<ChunkPos> pre_gen_chunks;
    cur_chunks.reserve(DISTANCE * DISTANCE);
    int half = DISTANCE / 2;
    for (int u = chunk_x - half; u <= chunk_x + half; ++u) {
        for (int v = chunk_z - half; v <= chunk_z + half; ++v) {
            cur_chunks.emplace(u, v);
        }
    }
    CUBED_ASSERT_MSG(!cur_chunks.empty(), "cur chunks is empty!!");

    for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
        if (cur_chunks.find(it->first) == cur_chunks.end()) {
            it = m_chunks.erase(it);
        } else {
            ++it;
        }
    }

    for (auto pos: cur_chunks) {
        auto it = m_chunks.find(pos);
        if (it == m_chunks.end()) {
            m_chunks.emplace(pos, Chunk(*this, pos));
            pre_gen_chunks.push_back(pos);
        }
    }
    if (pre_gen_chunks.empty()) {
        return;
    }
    static const ChunkPos CHUNK_DIR[] {
        {1, 0}, {-1, 0}, {0, -1}, {0, 1}
    };
    for (const auto& pos : pre_gen_chunks) {
        auto it = m_chunks.find(pos);
        CUBED_ASSERT_MSG(it != m_chunks.end(), "Chunk Don't find");
        //Logger::info("Init Chunk {} {}", pos.x, pos.z);
        it->second.init_chunk();
        it->second.mark_dirty();
        for (const auto& dir : CHUNK_DIR) {
            auto it = m_chunks.find(pos + dir);
            if (it != m_chunks.end()) {
                it->second.mark_dirty();
            }
        }
    }
    
}

void World::need_gen() {
    need_gen_chunk = true;
}

bool World::is_aabb_in_frustum(const glm::vec3& center, const glm::vec3& half_extents) {
    for (const auto& plane : m_planes) {
        // distance
        float d = glm::dot(glm::vec3(plane), center) + plane.w;
        float r = half_extents.x * std::abs(plane.x) +
                  half_extents.y * std::abs(plane.y) +
                  half_extents.z * std::abs(plane.z);
        if (d + r < 0) {
            return false;
        }
    }
    return true;
}
bool World::is_block(const glm::ivec3& block_pos) const{
    
    int world_x, world_y, world_z;
    world_x = block_pos.x;
    world_y = block_pos.y;
    world_z = block_pos.z;
    auto [chunk_x, chunk_z] = chunk_pos(world_x, world_z);

    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return false;
    }

    const auto& chunk_blocks = it->second.get_chunk_blocks();
    int x, y, z;
    y = world_y;
    x = world_x - chunk_x * CHUCK_SIZE;
    z = world_z - chunk_z * CHUCK_SIZE;
    if (x < 0 || y < 0 || z < 0 || x >= CHUCK_SIZE || y >= WORLD_SIZE_Y || z >= CHUCK_SIZE) {
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

    
    int world_x, world_y, world_z;
    world_x = block_pos.x;
    world_y = block_pos.y;
    world_z = block_pos.z;
    auto [chunk_x, chunk_z] = chunk_pos(world_x, world_z);

    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return ;
    }

    int x, y, z;
    y = world_y;
    x = world_x - chunk_x * CHUCK_SIZE;
    z = world_z - chunk_z * CHUCK_SIZE;
    if (x < 0 || y < 0 || z < 0 || x >= CHUCK_SIZE || y >= WORLD_SIZE_Y || z >= CHUCK_SIZE) {
        return ;
    }

    it->second.set_chunk_block(Chunk::get_index(x, y, z), id);

    static const glm::ivec3 NEIGHBOR_DIRS[] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, 0, 1}
    };

    for (const auto& dir : NEIGHBOR_DIRS) {
        glm::ivec3 neighbor = block_pos + dir;

        auto [cx, cz] = chunk_pos(neighbor.x, neighbor.z);
        auto it = m_chunks.find({cx, cz});
        if (it != m_chunks.end()) {
            it->second.mark_dirty();

        }
    }


}

void World::update(float delta_time) {
    for (auto& player : m_players) {
        player.second.update(delta_time);
    }
    if (need_gen_chunk) {
        gen_chunks();
        need_gen_chunk = false;
    }
    
    // unified compute vertex data before rendering
    for (auto& [key, chunk] : m_chunks) {
        if (chunk.is_dirty()) {
            chunk.gen_vertex_data();
            chunk.clear_dirty();
        }
    }
}