#include "Cubed/gameplay/world.hpp"

#include "Cubed/config.hpp"
#include "Cubed/gameplay/player.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/cubed_hash.hpp"

#include <execution>
#include <glm/gtc/constants.hpp>
#include <numbers>
using namespace std::chrono;

namespace Cubed {

struct ChunkRenderData {
    std::array<const std::vector<BlockType>*, 4> neighbor_block;
    Chunk* chunk;
};

World::World() {}

World::~World() {
    stop_gen_thread();
    stop_server_thread();
    m_chunks.clear();
    {
        std::lock_guard lk(m_delete_vbo_mutex);
        for (auto x : m_pending_delete_vbo) {
            glDeleteBuffers(1, &x);
        }
        m_pending_delete_vbo.clear();
    }
    {
        std::lock_guard lk(m_delete_vao_mutex);
        for (auto x : m_pending_delete_vao) {
            glDeleteVertexArrays(1, &x);
        }
        m_pending_delete_vao.clear();
    }
}

bool World::can_move(const AABB& player_box) const { return true; }

const std::optional<LookBlock>&
World::get_look_block_pos(const std::string& name) const {
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

Player& World::get_player(const std::string& name) {
    auto it = m_players.find(HASH::str(name));
    if (it == m_players.end()) {
        Logger::error("Can't find player {}", name);
        ASSERT(0);
    }

    return it->second;
}

void World::init_world() {
    m_cave_carcer.init(ChunkGenerator::seed());
    m_river_worm.init(ChunkGenerator::seed());
    m_chunks.reserve(MAX_DISTANCE * MAX_DISTANCE * 4);
    auto t1 = std::chrono::system_clock::now();

    Logger::info("Max Support Thread is {}",
                 std::thread::hardware_concurrency());
    // init players
    m_players.emplace(HASH::str("TestPlayer"), Player(*this, "TestPlayer"));

    start_gen_thread();
    init_chunks();
    auto t2 = std::chrono::system_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    Logger::info("Chunk Block Init Finish, Time Consuming: {}", d);

    start_server_thread();

    Logger::info("TestPlayer Create Finish");
}
void World::init_chunks() {
    hot_reload();
    while (!m_chunk_gen_finished) {
        // Logger::info("World Spawn: {:.2f}%", m_chunk_gen_fraction.load());
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}

ChunkPos World::chunk_pos(int world_x, int world_z) {
    int chunk_x, chunk_z;
    if (world_x < 0) {
        chunk_x = (world_x + 1) / CHUNK_SIZE - 1;
    }
    if (world_x >= 0) {
        chunk_x = world_x / CHUNK_SIZE;
    }
    if (world_z < 0) {
        chunk_z = (world_z + 1) / CHUNK_SIZE - 1;
    }
    if (world_z >= 0) {
        chunk_z = world_z / CHUNK_SIZE;
    }
    return {chunk_x, chunk_z};
}

#pragma region ChunkGenerate

void World::gen_chunks_internal() {
    m_chunk_gen_fraction = 0.0f;
    m_chunk_gen_finished = false;
    ChunkPosSet required_chunks;
    ChunkPairVector temp_neighbor;
    std::vector<ChunkPos> need_gen_temp_chunks_pos;
    compute_required_chunks(required_chunks, temp_neighbor,
                            need_gen_temp_chunks_pos);

    ASSERT_MSG(!required_chunks.empty(), "required chunks is empty!!");

    std::vector<ChunkPos> need_gen_chunks_pos;

    sync_and_collect_missing_chunks(need_gen_chunks_pos, required_chunks);

    Logger::info("New Gen Chunks Sum: {}", need_gen_chunks_pos.size());
    Logger::info("Temp Chunks sum {}", temp_neighbor.size());
    if (need_gen_chunks_pos.empty()) {
        m_could_gen = true;
        m_chunk_gen_fraction = 1.0f;
        return;
    }

    m_chunk_gen_fraction = 0.1f;

    ChunkPairVector new_chunks;
    ChunkPairVector new_temp_chunks;
    for (auto& pos : need_gen_chunks_pos) {
        new_chunks.push_back({pos, Chunk(*this, pos)});
    }
    for (auto& pos : need_gen_temp_chunks_pos) {
        new_temp_chunks.push_back({pos, Chunk(*this, pos)});
    }
    ConstChunkMap new_chunks_neighbor;

    build_neighbor_context_for_new_chunks(new_chunks_neighbor, new_chunks);

    std::for_each(std::execution::par, new_temp_chunks.begin(),
                  new_temp_chunks.end(),
                  [this](std::pair<ChunkPos, Chunk>& new_chunk) {
                      auto& [pos, chunk] = new_chunk;
                      chunk.gen_phase_one();
                      m_cave_carcer.try_to_add_path(pos, chunk.seed());
                      m_river_worm.try_to_add_path(pos, chunk.seed());
                  });

    std::for_each(std::execution::par, new_chunks.begin(), new_chunks.end(),
                  [](std::pair<ChunkPos, Chunk>& new_chunk) {
                      auto& [pos, chunk] = new_chunk;
                      chunk.gen_chunk();
                  });

    m_chunk_gen_fraction = 0.9f;

    {
        std::lock_guard lk(m_new_chunk_queue_mutex);
        for (auto& x : new_chunks) {
            m_new_chunk_queue.emplace_back(std::move(x));
        }
    }
    m_cave_carcer.cleanup_finished_caves();
    m_river_worm.cleanup_finished_rivers();
    m_chunk_gen_fraction = 1.0f;
    m_chunk_gen_finished = true;
}

void World::sync_player_pos(glm::vec3& player_pos) {
    std::lock_guard lk(m_gen_player_pos_mutex);
    player_pos = m_gen_player_pos;
}

void World::compute_required_chunks(
    ChunkPosSet& required_chunks, ChunkPairVector& temp_neighbor,
    std::vector<ChunkPos>& need_gen_temp_chunks_pos) {
    glm::vec3 player_pos;
    sync_player_pos(player_pos);

    int x = std::floor(player_pos.x);
    int z = std::floor(player_pos.z);
    auto [chunk_x, chunk_z] = chunk_pos(x, z);
    int radius = m_rendering_distance;
    int r2 = radius * radius;
    required_chunks.reserve(radius * radius);

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            if (dx * dx + dz * dz <= r2) {
                required_chunks.emplace(chunk_x + dx, chunk_z + dz);
            }
        }
    }
    int new_radius = radius + 1;
    int new_r2 = new_radius * new_radius;
    for (int dx = -new_radius; dx <= new_radius; ++dx) {
        for (int dz = -new_radius; dz <= new_radius; ++dz) {
            if (dx * dx + dz * dz <= new_r2) {
                int nx = chunk_x + dx;
                int nz = chunk_z + dz;

                need_gen_temp_chunks_pos.push_back({nx, nz});
            }
        }
    }
    int max_path_len = std::max(CavePath::step_max(), RiverPath::step_max());
    radius = max_path_len / 2;
    r2 = radius * radius;
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            if (dx * dx + dz * dz <= r2) {
                ChunkPos pos{chunk_x + dx, chunk_z + dz};
                auto it = required_chunks.find(pos);
                if (it != required_chunks.end()) {
                    continue;
                }
                temp_neighbor.emplace_back(pos, Chunk(*this, pos));
            }
        }
    }
}

void World::sync_and_collect_missing_chunks(
    std::vector<ChunkPos>& need_gen_chunks_pos,
    const ChunkPosSet& required_chunks) {
    std::lock_guard lk(m_chunks_mutex);
    for (auto it = m_chunks.begin(); it != m_chunks.end();) {
        if (required_chunks.find(it->first) == required_chunks.end()) {
            it = m_chunks.erase(it);
        } else {
            ++it;
        }
    }

    for (auto pos : required_chunks) {
        auto it = m_chunks.find(pos);
        if (it == m_chunks.end()) {
            need_gen_chunks_pos.push_back(pos);
        }
    }
}

void World::build_neighbor_context_for_new_chunks(
    ConstChunkMap& new_chunks_neighbor, const ChunkPairVector& new_chunks) {
    {
        std::lock_guard lk(m_chunks_mutex);
        for (auto& [pos, chunk] : new_chunks) {
            for (auto& dir : CHUNK_DIR) {
                auto it = m_chunks.find(pos + dir);
                if (it != m_chunks.end()) {
                    new_chunks_neighbor.insert({it->first, &(it->second)});
                }
            }
        }
    }
    for (auto& [pos, chunk] : new_chunks) {
        new_chunks_neighbor.insert({pos, &chunk});
    }
}

#pragma endregion

void World::start_gen_thread() {
    m_gen_running = true;
    Logger::info("Gen Thread Started");
    m_gen_thread = std::thread([this]() {
        while (m_gen_running) {
            std::unique_lock<std::mutex> lk(m_gen_signal_mutex);

            m_gen_cv.wait(lk, [this]() {
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

void World::start_server_thread() {
    m_server_thread = std::thread(
        [this]() { serever_run(m_server_stop_source.get_token()); });
}

void World::stop_gen_thread() {
    m_gen_running = false;
    m_gen_cv.notify_all();
    if (m_gen_thread.joinable()) {
        m_gen_thread.join();
    }
    Logger::info("Gen Thread Stopped");
}

void World::stop_server_thread() {
    m_server_stop_source.request_stop();
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }
}

void World::serever_run(std::stop_token stoken) {
    Logger::info("Server Thread Started!");
    while (!stoken.stop_requested()) {
        std::this_thread::sleep_for(milliseconds(m_per_tick_time));
        if (m_tick_running) {
            ++m_game_ticks;
            m_day_tick = (m_day_tick + 1) % DAY_TIME;
        }
    }
    Logger::info("Server Thread Stopped!");
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

int World::get_block(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = chunk_pos(block_pos.x, block_pos.z);
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return 0;
    }

    const auto& chunk_blocks = it->second.get_chunk_blocks();
    auto [x, y, z] = Chunk::world_to_block(block_pos, {chunk_x, chunk_z});
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return 0;
    }
    return chunk_blocks[Chunk::index(x, y, z)];
}

bool World::is_solid(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = chunk_pos(block_pos.x, block_pos.z);
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return false;
    }
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    auto [x, y, z] = Chunk::world_to_block(block_pos, {chunk_x, chunk_z});
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return false;
    }
    auto id = chunk_blocks[Chunk::index(x, y, z)];
    if (BlockManager::is_gas(id) || BlockManager::is_liquid(id)) {
        return false;
    } else {
        return true;
    }
}

bool World::can_pass_block(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = chunk_pos(block_pos.x, block_pos.z);
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return true;
    }
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    auto [x, y, z] = Chunk::world_to_block(block_pos, {chunk_x, chunk_z});
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return true;
    }
    auto id = chunk_blocks[Chunk::index(x, y, z)];
    return BlockManager::is_passable(id);
}

BlockType World::get_block_tpye(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = chunk_pos(block_pos.x, block_pos.z);
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        Logger::error("Can't Find Block {} {} {}", block_pos.x, block_pos.y,
                      block_pos.z);
        return 0;
    }
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    auto [x, y, z] = Chunk::world_to_block(block_pos, {chunk_x, chunk_z});
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        Logger::error("Can't Find Block {} {} {}", block_pos.x, block_pos.y,
                      block_pos.z);
        return 0;
    }
    return chunk_blocks[Chunk::index(x, y, z)];
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
        return;
    }

    auto [x, y, z] =
        Chunk::world_to_block(world_x, world_y, world_z, chunk_x, chunk_z);
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return;
    }

    it->second.set_chunk_block(Chunk::index(x, y, z), id);

    static const glm::ivec3 NEIGHBOR_DIRS[] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, 0, 1}};

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
        std::lock_guard lk(m_delete_vao_mutex);
        for (auto x : m_pending_delete_vao) {
            glDeleteVertexArrays(1, &x);
        }
        m_pending_delete_vao.clear();
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
                OptionalBlockVectorArray neighbor_block;
                for (int i = 0; i < 4; i++) {
                    auto it = m_chunks.find(pos + CHUNK_DIR[i]);
                    if (it != m_chunks.end()) {
                        neighbor_block[i] = (it->second.get_chunk_blocks());
                    } else {
                        neighbor_block[i] = std::nullopt;
                    }
                }
                chunk.gen_vertex_data(neighbor_block);
                chunk.upload_to_gpu();
            }
            if (!chunk.is_dirty()) {
                if (chunk.is_need_upload()) {
                    chunk.upload_to_gpu();
                }
                m_render_snapshots.push_back(
                    {chunk.get_normal_vao(), chunk.get_normal_vertices_sum(),
                     chunk.get_cross_vao(), chunk.get_cross_vertices_sum(),
                     chunk.get_normal_discard_vao(),
                     chunk.get_normal_discard_vertices_sum(),
                     chunk.get_normal_blend_vao(),
                     chunk.get_normal_blend_vertices_sum(),
                     chunk.get_water_vao(), chunk.get_water_vertices_sum(),
                     glm::vec3(static_cast<float>(pos.x * CHUNK_SIZE) +
                                   static_cast<float>(CHUNK_SIZE / 2),
                               static_cast<float>(WORLD_SIZE_Y / 2),
                               static_cast<float>(pos.z * CHUNK_SIZE) +
                                   static_cast<float>(CHUNK_SIZE / 2)),
                     glm::vec3(static_cast<float>(CHUNK_SIZE / 2),
                               static_cast<float>(WORLD_SIZE_Y / 2),
                               static_cast<float>(CHUNK_SIZE / 2))});
            }
        }
    }
}

void World::push_delete_vbo(GLuint vbo) {
    std::lock_guard lk(m_delete_vbo_mutex);
    m_pending_delete_vbo.push_back(vbo);
}

void World::push_delete_vao(GLuint vao) {
    std::lock_guard lk(m_delete_vao_mutex);
    m_pending_delete_vao.push_back(vao);
}

void World::hot_reload() {
    auto& config = Config::get();
    int dist = config.get<int>("world.rendering_distance");
    m_rendering_distance = dist <= MAX_DISTANCE ? dist : MAX_DISTANCE;
    need_gen();
}

void World::rebuild_world() {
    if (m_is_rebuilding) {
        return;
    }
    m_is_rebuilding = true;
    stop_gen_thread();
    m_cave_carcer.reload(ChunkGenerator::seed());
    m_river_worm.reload(ChunkGenerator::seed());
    {
        std::scoped_lock lk(m_chunks_mutex, m_new_chunk_queue_mutex);
        m_chunks.clear();
        m_new_chunk_queue.clear();
    }
    m_could_gen = true;
    ChunkGenerator::reload();
    start_gen_thread();
    need_gen();

    m_is_rebuilding = false;
}

float World::chunk_gen_fraction() const { return m_chunk_gen_fraction.load(); }

int World::rendering_distance() const { return m_rendering_distance.load(); }

void World::rendering_distance(int rendering_distance) {
    m_rendering_distance = rendering_distance;
}

CaveCarver& World::cave_carcer() { return m_cave_carcer; }
RiverWorm& World::river_worm() { return m_river_worm; }
std::vector<glm::vec4>& World::planes() { return m_planes; }
std::vector<ChunkRenderSnapshot>& World::render_snapshots() {
    return m_render_snapshots;
};
/*
glm::vec3 World::sunlight_dir() const {
    float t = static_cast<float>(m_day_tick) / DAY_TIME;

    float azimuth = glm::radians(90.0f - t * 360.0f);

    float altitude =
        glm::half_pi<float>() * sin((t - 0.25f) * glm::two_pi<float>());

    glm::vec3 dir{cos(altitude) * cos(azimuth), sin(altitude),
                  cos(altitude) * sin(azimuth)};

    return glm::normalize(-dir);
}
*/

glm::vec3 World::sunlight_dir() const {
    float altitude = sin((m_day_tick - 6 * PER_HOUR) /
                         static_cast<float>(DAY_TIME / 2) * std::numbers::pi) *
                     90.0f;

    float t = static_cast<float>(m_day_tick) / DAY_TIME;
    float azimuth = 90.0f - 360.0f * (t - 0.25f);

    float alt = glm::radians(altitude);
    float az = glm::radians(azimuth);
    glm::vec3 dir;
    dir.x = cos(alt) * sin(az);
    dir.y = sin(alt);
    dir.z = cos(alt) * cos(az);

    return glm::normalize(-dir);
}

TickType World::game_tick() const { return m_game_ticks.load(); }
TickType World::day_tick() const { return m_day_tick.load(); }
void World::day_tick(TickType tick) {
    tick %= DAY_TIME;
    m_day_tick = tick;
}
int World::per_tick_time() const { return m_per_tick_time.load(); }
void World::per_tick_time(int ms) { m_per_tick_time = ms; }

bool World::is_tick_running() const { return m_tick_running.load(); }
void World::tick_running(bool run) { m_tick_running = run; }
} // namespace Cubed