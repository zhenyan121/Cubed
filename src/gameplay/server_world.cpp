#include "Cubed/gameplay/server_world.hpp"

#include "Cubed/config.hpp"
#include "Cubed/gameplay/packet.hpp"
#include "Cubed/gameplay/session.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/uuid.hpp"

#include <utility>
using namespace std::chrono;
using namespace std::chrono_literals;

namespace Cubed {
ServerWorld::ServerWorld() {}

ServerWorld::~ServerWorld() {
    stop_gen_thread();
    stop_server_thread();
    wait_all_chunk_tasks();
    stop_thread_pool();
    {
        std::lock_guard lock(m_chunks_mutex);
        m_chunks.clear();
    }
}

void ServerWorld::wait_all_chunk_tasks() {
    std::lock_guard lock(m_new_chunk_mutex);
    for (auto& [pos, task] : m_new_chunks) {
        task.future.get();
    }
}

void ServerWorld::send_time() {
    UpdateTime rsp;
    rsp.set_day_tick(m_day_tick);
    rsp.set_game_tick(m_game_ticks);

    for (auto& [uuid, player] : m_players) {
        player.get_session()->send(make_packet(rsp));
    }
}

void ServerWorld::init_world() {
    m_cave_carcer.init(ChunkGenerator::seed());
    m_river_worm.init(ChunkGenerator::seed());
    m_chunks.reserve(MAX_DISTANCE * MAX_DISTANCE * 4);
    start_thread_pool();

    auto t1 = std::chrono::system_clock::now();

    // init players
    // m_players.emplace(HASH::str("TestPlayer"), Player(*this, "TestPlayer"));

    start_gen_thread();
    init_chunks();
    auto t2 = std::chrono::system_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    Logger::info("Chunk Block Init Finish, Time Consuming: {}", d);

    start_server_thread();
}

void ServerWorld::init_chunks() { hot_reload(); }

void ServerWorld::gen_chunks_internal(std::optional<std::string> uuid) {
    // Logger::info("gen_chunks_internal");
    m_chunk_gen_finished = false;

    ChunkPosSet required_chunks;
    compute_required_chunks(required_chunks, uuid);

    ASSERT_MSG(!required_chunks.empty(), "required chunks is empty!!");

    std::vector<ChunkPos> need_gen_chunks_pos;

    sync_and_collect_missing_chunks(need_gen_chunks_pos, required_chunks);

    Logger::info("New Gen Chunks Sum: {}", need_gen_chunks_pos.size());

    if (need_gen_chunks_pos.empty()) {
        m_could_gen = true;

        return;
    }
    {
        std::lock_guard lock(m_new_chunk_mutex);
        for (auto& pos : need_gen_chunks_pos) {
            m_new_chunks.emplace(pos, ServerChunk(*this, pos));
        }
    }

    submit_new_chunks(uuid);
    m_chunk_gen_finished = true;
}

void ServerWorld::compute_required_chunks(
    ChunkPosSet& required_chunks, const std::optional<std::string>& uuid) {
    glm::vec3 player_pos;
    if (uuid == std::nullopt) {
        player_pos = glm::vec3{0.0f};
    } else {
        player_pos = get_player_pos(uuid.value());
    }
    int x = std::floor(player_pos.x);
    int z = std::floor(player_pos.z);
    auto [chunk_x, chunk_z] = get_chunk_pos(x, z);
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
}
void ServerWorld::sync_and_collect_missing_chunks(
    std::vector<ChunkPos>& need_gen_chunks_pos,
    const ChunkPosSet& required_chunks) {
    std::lock_guard lk(m_chunks_mutex);
    for (auto it = m_chunks.begin(); it != m_chunks.end();) {
        if (required_chunks.find(it->first) == required_chunks.end()) {
            it = m_chunks.unsafe_erase(it);
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
void ServerWorld::submit_new_chunks(const std::optional<std::string>& uuid) {
    using enum ChunkLoadStyle;
    std::lock_guard lock(m_new_chunk_mutex);
    auto pool_ptr = m_gen_thread_pool.load();
    if (!pool_ptr) {
        return;
    }
    switch (m_chunk_load_style) {
    case RANDOM:
        for (auto& [pos, task] : m_new_chunks) {
            if (!task.future.valid()) {
                task.future =
                    pool_ptr->enqueue([&task]() { task.chunk.gen_chunk(); });
            }
        }
        break;
    case CENTER: {
        std::vector<std::pair<ChunkPos, PendingChunk*>> tasks;
        for (auto& [pos, task] : m_new_chunks) {
            if (!task.future.valid()) {
                tasks.emplace_back(pos, &task);
            }
        }
        glm::vec3 player_pos;
        if (uuid == std::nullopt) {
            player_pos = glm::vec3{0.0f};
        } else {
            player_pos = get_player_pos(uuid.value());
        }
        auto dist2 = [player_pos](ChunkPos chunk_pos) {
            ChunkPos player_chunk_pos =
                get_chunk_pos(player_pos.x, player_pos.z);
            float dx = player_chunk_pos.x - chunk_pos.x;
            float dz = player_chunk_pos.z - chunk_pos.z;
            return dx * dx + dz * dz;
        };

        std::sort(tasks.begin(), tasks.end(),
                  [&dist2](const auto& a, const auto& b) {
                      return dist2(a.first) < dist2(b.first);
                  });
        for (auto& [pos, task] : tasks) {
            if (!task->future.valid()) {
                task->future =
                    pool_ptr->enqueue([task]() { task->chunk.gen_chunk(); });
            }
        }
    }
    }
}

void ServerWorld::poll_finished_chunks() {
    m_new_finished_chunk.clear();
    std::lock_guard lock(m_new_chunk_mutex);
    std::erase_if(
        m_new_chunks, [&](std::pair<const ChunkPos, PendingChunk>& pair) {
            auto& pending = pair.second;
            if (!pending.future.valid()) {
                return false;
            }
            if (pending.future.wait_for(0ms) != std::future_status::ready) {
                return false;
            }
            pending.future.get();

            m_new_finished_chunk.emplace_back(pair.first,
                                              std::move(pending.chunk));
            return true;
        });
}

void ServerWorld::start_gen_thread() {
    m_gen_running = true;
    Logger::info("Gen Thread Started");
    m_gen_thread = std::jthread([this](std::stop_token token) {
        while (!token.stop_requested()) {
            std::unique_lock<std::mutex> lk(m_need_gen_queue_mutex);

            m_gen_cv.wait(lk, token, [this]() {
                return m_need_gen_chunk.load() || !m_gen_running ||
                       !m_need_gen_queue.empty();
            });
            if (!m_gen_running) {
                break;
            }
            if (token.stop_requested()) {
                break;
            }
            m_need_gen_chunk = false;
            std::optional<std::string> uuid{std::nullopt};
            if (!m_need_gen_queue.empty()) {
                uuid = m_need_gen_queue.front();
                m_need_gen_queue.pop_front();
            }
            lk.unlock();
            gen_chunks_internal(uuid);
        }
    });
}

void ServerWorld::start_server_thread() {
    m_server_thread =
        std::jthread([this](std::stop_token token) { serever_run(token); });
}

void ServerWorld::start_thread_pool() {
    int max_thread = std::thread::hardware_concurrency();
    if (m_pool_threads == 0) {
        change_pool_threads(max_thread - RESERVED_THREADS);
    } else {
        change_pool_threads(m_pool_threads);
    }
}

void ServerWorld::stop_gen_thread() {
    m_gen_running = false;
    m_gen_cv.notify_all();
    m_gen_thread.request_stop();
    if (m_gen_thread.joinable()) {
        m_gen_thread.join();
    }
    Logger::info("Gen Thread Stopped");
}

void ServerWorld::stop_server_thread() {
    m_server_thread.request_stop();
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }
}

void ServerWorld::stop_thread_pool() {
    auto pool_ptr = m_gen_thread_pool.load();
    if (pool_ptr) {
        pool_ptr->stop();
    }
    m_gen_thread_pool.store(nullptr);
    Logger::info("Thread Pool Stopped");
}

void ServerWorld::serever_run(std::stop_token stoken) {
    Logger::info("Server Thread Started!");
    while (!stoken.stop_requested()) {
        auto t1 = system_clock::now();

        if (m_tick_running) {
            ++m_game_ticks;
            m_day_tick = (m_day_tick + 1) % DAY_TIME;
        }
        update();
        auto t2 = system_clock::now();
        auto dt = duration_cast<microseconds>(t2 - t1);
        auto st = std::max(dt, milliseconds(m_per_tick_time) - dt);
        std::this_thread::sleep_for(st);
    }
    Logger::info("Server Thread Stopped!");
}

void ServerWorld::need_gen(std::optional<std::string> uuid) {

    // if (!m_could_gen) {
    //     Logger::warn("It is generating or consuming new chunks");
    //     return;
    // }

    m_could_gen = false;

    if (uuid) {
        std::lock_guard lock(m_need_gen_queue_mutex);
        m_need_gen_queue.push_back(*uuid);
    }

    // m_gen_player_pos = get_player("TestPlayer").get_player_pos();

    m_need_gen_chunk = true;

    m_gen_cv.notify_one();
}

bool ServerWorld::set_block(const glm::ivec3& block_pos, unsigned id) {

    int world_x, world_y, world_z;
    world_x = block_pos.x;
    world_y = block_pos.y;
    world_z = block_pos.z;

    auto [chunk_x, chunk_z] = get_chunk_pos(world_x, world_z);
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return false;
    }

    auto [x, y, z] = ServerChunk::world_to_block(world_x, world_y, world_z,
                                                 chunk_x, chunk_z);
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return false;
    }

    it->second.set_chunk_block(ServerChunk::index(x, y, z), id);
    return true;
}

void ServerWorld::hot_reload() {
    auto& config = Config::get();
    int dist = config.get<int>("world.rendering_distance");
    m_rendering_distance = dist <= MAX_DISTANCE ? dist : MAX_DISTANCE;
    need_gen(std::nullopt);
}

void ServerWorld::rebuild_world() {
    if (m_is_rebuilding.exchange(true)) {
        return;
    }
    stop_gen_thread();
    stop_thread_pool();
    m_cave_carcer.reload(ChunkGenerator::seed());
    m_river_worm.reload(ChunkGenerator::seed());
    {
        std::lock_guard lk(m_chunks_mutex);
        m_chunks.clear();
        m_new_finished_chunk.clear();
    }
    {
        std::lock_guard lock(m_new_chunk_mutex);
        m_new_chunks.clear();
    }
    m_could_gen = true;
    ChunkGenerator::reload();
    start_thread_pool();
    start_gen_thread();
    need_gen(std::nullopt);
    m_is_rebuilding = false;
}

void ServerWorld::update() {
    poll_finished_chunks();
    {
        std::lock_guard lk(m_chunks_mutex);
        bool consumed = false;

        for (auto& x : m_new_finished_chunk) {
            m_chunks.emplace(x.first, std::move(x.second));
            consumed = true;
        }
        if (consumed) {
            m_could_gen = true;
        }
    }
    send_time();
    for (auto& [id, timer] : m_timers) {
        timer.update();
    }
}

void ServerWorld::sync_player_pos(const std::string& uuid, float x, float y,
                                  float z) {
    std::lock_guard lock(m_player_mutex);
    auto it = m_players.find(uuid);
    if (it == m_players.end()) {
        Logger::warn("Player {} is not in this Server", uuid);
        return;
    }
    it->second.update_pos(x, y, z);
}

void ServerWorld::handle_player_login(const std::string& name,
                                      std::shared_ptr<Session> session) {
    std::string uuid = generate_uuid();
    Logger::info("Player {} (uuid {}) join the world", name, uuid);
    {
        std::lock_guard lock(m_player_mutex);
        m_players.emplace(std::piecewise_construct,
                          std::forward_as_tuple(std::string(uuid)),
                          std::forward_as_tuple(name, uuid, *this, session));
    }
    m_uuid_to_name.emplace(uuid, name);
    LoginRsp rsp;
    rsp.set_success(true);
    rsp.set_uuid(uuid);
    session->send(make_packet(rsp));
}

void ServerWorld::handle_player_exit(const std::string& uuid) {
    {
        std::lock_guard lock(m_player_mutex);
        auto it = m_players.find(uuid);
        if (it != m_players.end()) {

            m_players.erase(it);
        } else {
            Logger::error("Player {} isn't in Server", uuid);
        }
    }
    m_uuid_to_name.erase(uuid);
}

glm::vec3 ServerWorld::get_player_pos(const std::string& uuid) const {
    std::shared_lock lock(m_player_mutex);
    auto it = m_players.find(uuid);
    if (it == m_players.end()) {
        Logger::error("Can't find player uuid {}", uuid);
        return glm::vec3{0.0f};
    }
    return it->second.get_pos();
}

void ServerWorld::handle_chunk_req(const std::string& uuid, ChunkPos pos) {
    ChunkDataRsp rsq;
    auto* rsq_pos = rsq.mutable_pos();
    rsq_pos->set_x(pos.x);
    rsq_pos->set_z(pos.z);
    {
        std::shared_lock lock(m_chunks_mutex);
        auto it = m_chunks.find(pos);
        if (it == m_chunks.end()) {
            return;
        }
        rsq.set_chunk_seed(it->second.seed());
        rsq.set_biome_type(std::to_underlying(it->second.biome()));
        auto* blocks = rsq.mutable_chunk_blocks();
        auto& chunk_blocks = it->second.get_chunk_blocks();
        blocks->Assign(chunk_blocks.begin(), chunk_blocks.end());
        auto& neighbor_blocks = it->second.get_neightbor_blocks();
        auto assign = [](auto* nb,
                         const std::optional<std::vector<BlockType>>& blocks) {
            if (!blocks) {
                return;
            }
            if (!nb) {
                return;
            }
            nb->Assign(blocks->begin(), blocks->end());
        };
        auto* nb1 = rsq.mutable_neighbor_blocks_1();
        auto* nb2 = rsq.mutable_neighbor_blocks_2();
        auto* nb3 = rsq.mutable_neighbor_blocks_3();
        auto* nb4 = rsq.mutable_neighbor_blocks_4();
        assign(nb1, neighbor_blocks[1]);
        assign(nb2, neighbor_blocks[2]);
        assign(nb3, neighbor_blocks[3]);
        assign(nb4, neighbor_blocks[4]);
    }
    std::shared_ptr<Session> s;
    {
        std::shared_lock lock(m_player_mutex);
        auto it = m_players.find(uuid);
        if (it != m_players.end()) {
            s = it->second.get_session();
        }
    }
    if (!s) {
        Logger::error("Player {} session not exist", uuid);
        return;
    }
    s->send(make_packet(rsq));
}

void ServerWorld::handle_block_change(const BlockChangeReq& req) {
    float x = req.pos().x();
    float y = req.pos().y();
    float z = req.pos().z();
    if (!set_block(glm::ivec3(x, y, z), req.block())) {
        return;
    }
    BlockChangeRsp rsp;
    auto* pos = rsp.mutable_pos();
    pos->set_x(x);
    pos->set_y(y);
    pos->set_z(z);
    rsp.set_block(req.block());
    {
        std::shared_lock lock(m_player_mutex);
        for (auto& [uuid, player] : m_players) {
            auto session = player.get_session();
            if (session) {
                session->send(make_packet(rsp));
            }
        }
    }
}

int ServerWorld::rendering_distance() const {
    return m_rendering_distance.load();
}

void ServerWorld::rendering_distance(int rendering_distance) {
    m_rendering_distance = rendering_distance;
}

CaveCarver& ServerWorld::cave_carcer() { return m_cave_carcer; }
RiverWorm& ServerWorld::river_worm() { return m_river_worm; }

TickType ServerWorld::game_tick() const { return m_game_ticks.load(); }
TickType ServerWorld::day_tick() const { return m_day_tick.load(); }
void ServerWorld::day_tick(TickType tick) {
    tick %= DAY_TIME;
    m_day_tick = tick;
}
int ServerWorld::per_tick_time() const { return m_per_tick_time.load(); }
void ServerWorld::per_tick_time(int ms) { m_per_tick_time = ms; }

bool ServerWorld::is_tick_running() const { return m_tick_running.load(); }
void ServerWorld::tick_running(bool run) { m_tick_running = run; }
int ServerWorld::pool_threads() const { return m_pool_threads.load(); }
int ServerWorld::max_threads() const { return m_max_threads.load(); }
void ServerWorld::change_pool_threads(int threads) {
    m_max_threads = std::thread::hardware_concurrency();
    if (m_max_threads < 1) {
        Logger::warn("Can't Get Max Support Threads, Set Max Threads to 4");
        m_max_threads = 4;
    }
    int used_thread = std::clamp(threads, 1, m_max_threads.load());
    Logger::info("Create New Thread Pool Use {} Threads", used_thread);
    m_gen_thread_pool.store(std::make_shared<ThreadPool>(used_thread));
    m_pool_threads = used_thread;
}
int ServerWorld::chunk_load_style() const {
    return std::to_underlying(m_chunk_load_style.load());
}
void ServerWorld::set_chunk_load_style(int id) {
    using enum ChunkLoadStyle;

    switch (id) {
    case std::to_underlying(RANDOM):
        m_chunk_load_style = RANDOM;
        return;
    case std::to_underlying(CENTER):
        m_chunk_load_style = CENTER;
        return;
    }
    Logger::error("Can,t Find Chunk Load Style Id {}, Nothing Will Do", id);
}

} // namespace Cubed