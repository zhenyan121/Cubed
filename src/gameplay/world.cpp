#include <Cubed/gameplay/world.hpp>

#include <Cubed/debug_collector.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/cubed_hash.hpp>
#include <Cubed/tools/math_tools.hpp>

#include <execution>

namespace Cubed {

static constexpr ChunkPos CHUNK_DIR[] {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    };

struct ChunkRenderData {
    std::array<const std::vector<uint8_t>*, 4> neighbor_block;
    Chunk* chunk;
};

World::World() {
    
}

World::~World() {
    stop_gen_thread();
    m_chunks.clear();
    {
        std::lock_guard lk(m_delete_vbo_mutex);
        for (auto x : m_pending_delete_vbo) {
            glDeleteBuffers(1, &x);
        }
        m_pending_delete_vbo.clear();
    }
}

bool World::can_move(const AABB& player_box) const{

    return true;
}

const std::optional<LookBlock>& World::get_look_block_pos(const std::string& name) const{
    static std::optional<LookBlock> null_look_block = std::nullopt;
    auto it = m_players.find(HASH::str(name));
    if (it == m_players.end()) {
        Logger::error("Can't find player {}", name);
        ASSERT(0);
        return null_look_block;
    }

    return it->second.get_look_block_pos();
    
}

const Chunk* World::get_chunk(const ChunkPos& pos) const {
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(pos);
    if (it == m_chunks.end()) {
        return nullptr;
    }
    return &it->second;
}

Player& World::get_player(const std::string& name){
    auto it = m_players.find(HASH::str(name));
    if (it == m_players.end()) {
        Logger::error("Can't find player {}", name);
        ASSERT(0);
    }
    
    return it->second;
}

void World::init_world() {
    m_chunks.reserve(DISTANCE * DISTANCE);
    auto t1 = std::chrono::system_clock::now();
    for (int s = 0; s < DISTANCE; s++) {
        for (int t = 0; t < DISTANCE; t++) {
            int ns = s - DISTANCE / 2;
            int nt = t - DISTANCE / 2;

            ChunkPos pos{ns, nt};

            m_chunks.emplace(pos, Chunk(*this, pos)); 
        }
    }

    Logger::info("Max Support Thread is {}", std::thread::hardware_concurrency());
    init_chunks();
    auto t2 = std::chrono::system_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    Logger::info("Chunk Block Init Finish, Time Consuming: {}", d);
    // init players
    m_players.emplace(HASH::str("TestPlayer"), Player(*this, "TestPlayer"));
    Logger::info("TestPlayer Create Finish");

    start_gen_thread();

}
/*
void World::init_chunks() {
    std::vector<Chunk*> chunk_ptrs;
    chunk_ptrs.reserve(m_chunks.size());
    for (auto& [pos, chunk] : m_chunks) {
        chunk_ptrs.push_back(&chunk);
    }

    std::for_each(std::execution::par, chunk_ptrs.begin(), chunk_ptrs.end(), [](auto& chunk){
        chunk->init_chunk();
    });

    std::atomic<int> sync{0};
    sync.store(1, std::memory_order_release);
    sync.load(std::memory_order_acquire);
    
    std::vector<ChunkRenderData> pending_gen_data;
    pending_gen_data.reserve(m_chunks.size());
    for (auto& [pos, chunk] : m_chunks) {
        ChunkRenderData data;
        data.chunk = &chunk;
        for (int i = 0; i < 4; i++) {    
            auto it = m_chunks.find(pos + CHUNK_DIR[i]);
            if (it != m_chunks.end()) {
                data.neighbor_block[i] = &(it->second.get_chunk_blocks());
            } else {
                data.neighbor_block[i] = nullptr;
            }
        }
        pending_gen_data.emplace_back(std::move(data));
    }
    std::for_each(std::execution::par, pending_gen_data.begin(), pending_gen_data.end(), [](ChunkRenderData& data){
        if(!data.chunk) {
            return ;
        }
        data.chunk->gen_vertex_data(data.neighbor_block);
    });
    for (auto& chunk_map : m_chunks) {
        auto& [chunk_pos, chunk] = chunk_map;
        chunk.upload_to_gpu();
        
    }
}
*/

void World::init_chunks() {
    for (auto& [pos, chunks] : m_chunks) {
        chunks.gen_phase_one();
    }
    std::array<const Chunk*, 4> neighbor_chunks;
    for (auto& [pos, chunks] : m_chunks) {
        for (int i = 0; i < 4; i++) {
            auto neighbor_pos = pos + CHUNK_DIR[i];
            auto it = m_chunks.find(neighbor_pos);
            if (it == m_chunks.end()) {
                neighbor_chunks[i] = nullptr;
                continue;
            }
            neighbor_chunks[i] = &it->second;
            
        }
        chunks.gen_phase_two(neighbor_chunks);
    }

    for (auto& [pos, chunks] : m_chunks) {
        chunks.gen_phase_three();
    }
    std::array<std::optional<HeightMapArray>, 4> neighbor_chunk_heightmap;
    for (auto& [pos, chunks] : m_chunks) {
        for (int i = 0; i < 4; i++) {
            auto neighbor_pos = pos + CHUNK_DIR[i];
            auto it = m_chunks.find(neighbor_pos);
            if (it == m_chunks.end()) {
                neighbor_chunk_heightmap[i] = std::nullopt;
                continue;
            }
            neighbor_chunk_heightmap[i] = it->second.get_heightmap();
            
        }
        chunks.gen_phase_four(neighbor_chunk_heightmap);
    }

    for (auto& [pos, chunks] : m_chunks) {
        chunks.gen_phase_five();
        chunks.gen_phase_six();
    }

    std::atomic<int> sync{0};
    sync.store(1, std::memory_order_release);
    sync.load(std::memory_order_acquire);
    
    std::vector<ChunkRenderData> pending_gen_data;
    pending_gen_data.reserve(m_chunks.size());
    for (auto& [pos, chunk] : m_chunks) {
        ChunkRenderData data;
        data.chunk = &chunk;
        for (int i = 0; i < 4; i++) {    
            auto it = m_chunks.find(pos + CHUNK_DIR[i]);
            if (it != m_chunks.end()) {
                data.neighbor_block[i] = &(it->second.get_chunk_blocks());
            } else {
                data.neighbor_block[i] = nullptr;
            }
        }
        pending_gen_data.emplace_back(std::move(data));
    }
    std::for_each(std::execution::par, pending_gen_data.begin(), pending_gen_data.end(), [](ChunkRenderData& data){
        if(!data.chunk) {
            return ;
        }
        data.chunk->gen_vertex_data(data.neighbor_block);
    });
    for (auto& chunk_map : m_chunks) {
        auto& [chunk_pos, chunk] = chunk_map;
        chunk.upload_to_gpu();
        
    }

}

void World::render(const glm::mat4& mvp_matrix) {
    Math::extract_frustum_planes(mvp_matrix, m_planes);
    int rendered_sum = 0;
    for (const auto& snapshot : m_render_snapshots) {
        
        if (is_aabb_in_frustum(snapshot.center, snapshot.half_extents)) {
            glBindBuffer(GL_ARRAY_BUFFER, snapshot.vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, s));
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, layer));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);

            glDrawArrays(GL_TRIANGLES, 0, snapshot.vertex_count);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            rendered_sum++;

        }
        

    }
    DebugCollector::get().report("rendered_chunk", "Rendered Chunk: " + std::to_string(rendered_sum));

}

ChunkPos World::chunk_pos(int world_x, int world_z) {
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

void World::gen_chunks_internal() {
    
    ChunkPosSet required_chunks;
    compute_required_chunks(required_chunks);

    ASSERT_MSG(!required_chunks.empty(), "required chunks is empty!!");
    
    std::vector<ChunkPos> need_gen_chunks_pos;
    sync_and_collect_missing_chunks(need_gen_chunks_pos, required_chunks);
    
    Logger::info("New Gen Chunks Sum: {}", need_gen_chunks_pos.size());
    if (need_gen_chunks_pos.empty()) {
        m_could_gen = true;
        return;
    }
    ChunkUpdateList new_chunks;
    for (auto& pos : need_gen_chunks_pos) {
        new_chunks.push_back({pos, Chunk(*this, pos)});
    }
    
    ConstChunkMap new_chunks_neighbor;
    // affected neighbor
    ChunkPtrUpdateList affected_neighbor;
    build_neighbor_context_for_new_chunks(new_chunks_neighbor, affected_neighbor,new_chunks);
    
    std::array<const std::vector<uint8_t>*, 4> neighbor_block;
    // build new chunk, but the neighbor in m_chunks also need to re-build

    for (auto& [pos, chunk] : new_chunks) {
        chunk.gen_phase_one();
    }

    std::array<const Chunk*, 4> neighbor_chunks;
    for (auto& [pos, chunks] : new_chunks) {
        for (int i = 0; i < 4; i++) {
            auto neighbor_pos = pos + CHUNK_DIR[i];
            auto it = new_chunks_neighbor.find(neighbor_pos);
            if (it == new_chunks_neighbor.end()) {
                neighbor_chunks[i] = nullptr;
                continue;
            }
            neighbor_chunks[i] = it->second;
            
        }
        chunks.gen_phase_two(neighbor_chunks);
    }

    for (auto& [pos, chunks] : new_chunks) {
        chunks.gen_phase_three();
    }
    std::array<std::optional<HeightMapArray>, 4> neighbor_chunk_heightmap;
    for (auto& [pos, chunks] : new_chunks) {
        {
            //std::lock_guard lk(m_chunks_mutex);
            for (int i = 0; i < 4; i++) {
                auto neighbor_pos = pos + CHUNK_DIR[i];
                auto it = new_chunks_neighbor.find(neighbor_pos);
                if (it == new_chunks_neighbor.end()) {
                    neighbor_chunk_heightmap[i] = std::nullopt;
                    continue;
                }
                neighbor_chunk_heightmap[i] = it->second->get_heightmap();
            }
        }
        chunks.gen_phase_four(neighbor_chunk_heightmap);
    }

    for (auto& [pos, chunks] : new_chunks) {
        chunks.gen_phase_five();
        chunks.gen_phase_six();
    }

    for (auto& [pos, chunk] : new_chunks) {
        for (int i = 0; i < 4; i++) {
            auto it = new_chunks_neighbor.find(pos + CHUNK_DIR[i]);
            if (it != new_chunks_neighbor.end()) {
                neighbor_block[i] = &(it->second->get_chunk_blocks());
            } else {
                neighbor_block[i] = nullptr;
            }
        }
        chunk.gen_vertex_data(neighbor_block);
    }
    build_neighbor_context_for_affected_neighbors(affected_neighbor, new_chunks_neighbor);

    for (auto& [pos, chunk] : affected_neighbor) {
        for (int i = 0; i < 4; i++) {
            auto it = new_chunks_neighbor.find(pos + CHUNK_DIR[i]);
            if (it != new_chunks_neighbor.end()) {
                neighbor_block[i] = &(it->second->get_chunk_blocks());
            } else {
                neighbor_block[i] = nullptr;
            }
        }
        chunk->gen_vertex_data(neighbor_block);
        chunk->need_upload();
    }    

    {
        std::lock_guard lk(m_new_chunk_queue_mutex);
        for (auto& x : new_chunks) {
            m_new_chunk_queue.emplace_back(std::move(x));
        }
        
    }
     
}

void World::sync_player_pos(glm::vec3& player_pos) {
    std::lock_guard lk(m_gen_player_pos_mutex);
    player_pos = m_gen_player_pos;
}

void World::compute_required_chunks(ChunkPosSet& required_chunks) {
    glm::vec3 player_pos;
    sync_player_pos(player_pos);
    
    int x = std::floor(player_pos.x);
    int z = std::floor(player_pos.z);
    auto [chunk_x, chunk_z] = chunk_pos(x, z);
    

    required_chunks.reserve(DISTANCE * DISTANCE);
    int half = DISTANCE / 2;
    for (int u = chunk_x - half; u <= chunk_x + half; ++u) {
        for (int v = chunk_z - half; v <= chunk_z + half; ++v) {
            required_chunks.emplace(u, v);
        }
    }
}

void World::sync_and_collect_missing_chunks(std::vector<ChunkPos>& need_gen_chunks_pos, const ChunkPosSet& required_chunks) {
    std::lock_guard lk(m_chunks_mutex);
    for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
        if (required_chunks.find(it->first) == required_chunks.end()) {
            it = m_chunks.erase(it);
        } else {
            ++it;
        }
    }
    
    for (auto pos: required_chunks) {
        auto it = m_chunks.find(pos);
        if (it == m_chunks.end()) {
            need_gen_chunks_pos.push_back(pos);
        }
    }
}

void World::build_neighbor_context_for_new_chunks(ConstChunkMap& new_chunks_neighbor, ChunkPtrUpdateList& affected_neighbor,const ChunkUpdateList& new_chunks) {
    {
        std::lock_guard lk(m_chunks_mutex);
        for (auto& [pos, chunk] : new_chunks) {
            for (auto& dir : CHUNK_DIR) {
                auto it = m_chunks.find(pos + dir);
                if (it != m_chunks.end()) {
                    new_chunks_neighbor.insert({it->first, &(it->second)});
                    affected_neighbor.push_back({it->first, &(it->second)});
                }
            }
        }
    }
    for (auto& [pos, chunk] : new_chunks) {
        new_chunks_neighbor.insert({pos, &chunk});
    }
}

void World::build_neighbor_context_for_affected_neighbors(ChunkPtrUpdateList& affected_neighbor, ConstChunkMap& new_chunks_neighbor) {
    std::lock_guard lk(m_chunks_mutex);
    for (auto& [pos, chunk] : affected_neighbor) {
        for (auto& dir : CHUNK_DIR) {
            auto it = m_chunks.find(pos + dir);
            if (it != m_chunks.end()) {
                new_chunks_neighbor.insert({it->first, &(it->second)});
            }
        }
    }
}

void World::start_gen_thread() {
    m_gen_running = true;
    Logger::info("Gen Thread Started");
    m_gen_thread = std::thread([this](){
        while (m_gen_running) {
            std::unique_lock<std::mutex> lk(m_gen_signal_mutex);

            m_gen_cv.wait(lk, [this](){
                return m_need_gen_chunk.load() || !m_gen_running;
            });
            if (!m_gen_running) {
                break;
            }
            m_need_gen_chunk = false;
            lk.unlock();

            gen_chunks_internal();
        }
    });
}

void World::stop_gen_thread() {
    m_gen_running = false;
    m_gen_cv.notify_all();
    if (m_gen_thread.joinable()) {
        m_gen_thread.join();
    }
    Logger::info("Gen Thread Stopped");
}

void World::need_gen() {
    if (!m_could_gen) {
        Logger::warn("It is generating or consuming new chunks");
        return;
    }
    m_could_gen = false;
    {
        std::lock_guard lk(m_gen_player_pos_mutex);
        m_gen_player_pos = get_player("TestPlayer").get_player_pos();
    }
    
    m_need_gen_chunk = true;
    m_gen_cv.notify_one();
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

int World::get_block(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = chunk_pos(block_pos.x, block_pos.z);
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return 0;
    }

    const auto& chunk_blocks = it->second.get_chunk_blocks();
    int x, y, z;
    y = block_pos.y;
    x = block_pos.x - chunk_x * CHUCK_SIZE;
    z = block_pos.z - chunk_z * CHUCK_SIZE;
    if (x < 0 || y < 0 || z < 0 || x >= CHUCK_SIZE || y >= WORLD_SIZE_Y || z >= CHUCK_SIZE) {
        return 0;
    }
    return chunk_blocks[Chunk::get_index(x, y, z)];

}

bool World::is_block(const glm::ivec3& block_pos) const{
    auto [chunk_x, chunk_z] = chunk_pos(block_pos.x, block_pos.z);
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return false;
    }
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    int x, y, z;
    y = block_pos.y;
    x = block_pos.x - chunk_x * CHUCK_SIZE;
    z = block_pos.z - chunk_z * CHUCK_SIZE;
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
    std::lock_guard lk(m_chunks_mutex);
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
    {
        std::lock_guard lk(m_delete_vbo_mutex);
        for (auto x : m_pending_delete_vbo) {
            glDeleteBuffers(1, &x);
        }
        m_pending_delete_vbo.clear();
    }
    {
        std::scoped_lock lk(m_chunks_mutex, m_new_chunk_queue_mutex);
        m_new_chunk.clear();
        for (auto& x : m_new_chunk_queue) {
            m_new_chunk.emplace_back(std::move(x));
        }
        m_new_chunk_queue.clear();
    }

    for (auto& x : m_new_chunk) {
        x.second.upload_to_gpu();
    }

    // unified compute vertex data before rendering
    {   
        std::lock_guard lk(m_chunks_mutex);
        bool consumed = false;
        
        for (auto& x : m_new_chunk) {
            m_chunks.insert_or_assign(x.first, std::move(x.second));
            consumed = true;
        }
        if (consumed) {
            m_could_gen = true;
        }
        
        m_render_snapshots.clear();
        for (auto& [pos, chunk] : m_chunks) {
            if (chunk.is_dirty()) {
                // the curial fator influence
                std::array<const std::vector<uint8_t>*, 4> neighbor_block;
                for (int i = 0; i < 4; i++) {
                    auto it = m_chunks.find(pos + CHUNK_DIR[i]);
                    if (it != m_chunks.end()) {
                        neighbor_block[i] = &(it->second.get_chunk_blocks());
                    } else {
                        neighbor_block[i] = nullptr;
                    }
                }
                chunk.gen_vertex_data(neighbor_block);
                chunk.upload_to_gpu();
            }
            if (!chunk.is_dirty()) {
                if (chunk.is_need_upload()) {
                    chunk.upload_to_gpu();
                }
                m_render_snapshots.push_back({
                    chunk.get_vbo(),
                    chunk.get_vertex_sum(),
                    glm::vec3(static_cast<float>(pos.x * CHUCK_SIZE) + static_cast<float>(CHUCK_SIZE / 2), static_cast<float>(WORLD_SIZE_Y/ 2), static_cast<float>(pos.z * CHUCK_SIZE) + static_cast<float>(CHUCK_SIZE / 2)),
                    glm::vec3(static_cast<float>(CHUCK_SIZE / 2), static_cast<float>(WORLD_SIZE_Y / 2), static_cast<float>(CHUCK_SIZE / 2))
                    }
                );
            }
            
        }
    }
}

void World::push_delete_vbo(GLuint vbo) {
    std::lock_guard lk(m_delete_vbo_mutex);
    m_pending_delete_vbo.push_back(vbo);
}

}