#include "Cubed/gameplay/server_world.hpp"

#include "Cubed/config.hpp"
#include "Cubed/gameplay/packet.hpp"
#include "Cubed/gameplay/session.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/uuid.hpp"

#include <ranges>
#include <utility>
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace google::protobuf;

namespace Cubed {
ServerWorld::ServerWorld() {}

ServerWorld::~ServerWorld() { stop(); }

void ServerWorld::stop() {
    if (!m_init) {
        return;
    }
    if (m_stopped.exchange(true)) {
        return;
    }
    send_server_stop();
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
        if (task.future.valid()) {
            try {
                task.future.get();
            } catch (const std::exception& e) {
                Logger::error("Chunk generation failed: {}", e.what());
                continue;
            }
        } else {
            Logger::error("Chunk {} {} not started gen task", pos.x, pos.z);
        }
    }
}

void ServerWorld::update_ref_count(const ChunkPosSet& old,
                                   const ChunkPosSet& now) {
    std::lock_guard lock(m_chunks_mutex);

    // Elements in the old set that are not contained in now are not needed by
    // the current player.

    for (auto& pos : old) {
        if (!now.contains(pos)) {
            auto it = m_chunks.find(pos);
            if (it == m_chunks.end()) {
                Logger::warn(
                    "Update Ref Count Error, can't Find old pos in m_chunks");
                continue;
            }
            if (it->second.ref_count == 0) {
                Logger::error("Chunk {} {} error, ref count is 0", pos.x,
                              pos.z);
                m_chunks.erase(pos);
                continue;
            }
            if (--it->second.ref_count == 0) {
                m_chunks.erase(pos);
            }
        }
    }

    for (auto& pos : now) {
        auto it = m_chunks.find(pos);
        if (it == m_chunks.end()) {
            Logger::warn(
                "Update Ref Count Error, can't Find now pos in m_chunks");
            continue;
        }
        if (!old.contains(pos)) {
            ++it->second.ref_count;
        }
    }
}

void ServerWorld::send_time() {
    Arena arena;
    auto* rsp = Arena::Create<UpdateTime>(&arena);

    rsp->set_day_tick(m_day_tick);
    rsp->set_game_tick(m_game_ticks);

    for (auto& [uuid, player] : m_players) {
        player.get_session()->send(make_packet(*rsp));
    }
}

void ServerWorld::send_chunk(int task_id, const std::string& uuid,
                             ChunkPos pos) {

    {
        std::shared_lock lock(m_player_mutex);
        auto it = m_players.find(uuid);
        if (it == m_players.end()) {
            return;
        }
        if (task_id < it->second.task_id()) {
            // Old chunk requests are simply discarded
            return;
        }
    }

    Arena arean;
    ChunkDataRsp* rsp = Arena::Create<ChunkDataRsp>(&arean);
    auto* rsq_pos = rsp->mutable_pos();
    rsq_pos->set_x(pos.x);
    rsq_pos->set_z(pos.z);
    {
        std::shared_lock lock(m_chunks_mutex);
        auto it = m_chunks.find(pos);
        if (it == m_chunks.end()) {
            // No chunk found and not generating
            Logger::error("Chunk {} {} neither pending nor ready", pos.x,
                          pos.z);
            return;
        }

        if (it->second.state == ChunkState::GENERATING) {

            m_waiting_chunk_requests.emplace(uuid, task_id, pos);
            return;
        }
        if (it->second.state != ChunkState::READY) {
            Logger::error("Chunk {} {} is invaild", pos.x, pos.z);
            return;
        }

        rsp->set_chunk_seed(it->second.chunk->seed());
        rsp->set_biome_type(std::to_underlying(it->second.chunk->biome()));
        auto* blocks = rsp->mutable_chunk_blocks();
        auto& chunk_blocks = it->second.chunk->get_chunk_blocks();
        blocks->Assign(chunk_blocks.begin(), chunk_blocks.end());
        auto& neighbor_blocks = it->second.chunk->get_neightbor_blocks();

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
        auto* nb1 = rsp->mutable_neighbor_blocks_1();
        auto* nb2 = rsp->mutable_neighbor_blocks_2();
        auto* nb3 = rsp->mutable_neighbor_blocks_3();
        auto* nb4 = rsp->mutable_neighbor_blocks_4();
        assign(nb1, neighbor_blocks[0]);
        assign(nb2, neighbor_blocks[1]);
        assign(nb3, neighbor_blocks[2]);
        assign(nb4, neighbor_blocks[3]);
    }
    std::shared_ptr<Session> s;
    {
        std::shared_lock lock(m_player_mutex);
        auto it = m_players.find(uuid);
        if (it != m_players.end()) {
            s = it->second.get_session();
            it->second.update_sync_gametick(m_game_ticks);
        }
    }
    if (!s) {
        Logger::error("Player {} session not exist", uuid);
        return;
    }
    rsp->set_task_id(task_id);
    s->send(make_packet(*rsp));
}

void ServerWorld::init_world() {

    register_timer("player disconnect", 5, [this]() {
        std::vector<std::string> disconnect;
        {
            std::shared_lock lock(m_player_mutex);
            for (auto& [uuid, player] : m_players) {
                if (player.is_disconnect(m_game_ticks)) {
                    disconnect.emplace_back(uuid);
                }
            }
        }
        for (auto& uuid : disconnect) {
            handle_player_exit(uuid);
        }
    });
    // Periodically process pending players
    register_timer("player chunk send", 1, [this]() {
        PendingRequest request;
        if (m_waiting_chunk_requests.try_pop(request)) {
            handle_chunk_req(request.task_id, request.uuid, request.pos);
        }
    });

    m_cave_carcer.init(ChunkGenerator::seed());
    m_river_worm.init(ChunkGenerator::seed());
    m_chunks.reserve(MAX_DISTANCE * MAX_DISTANCE * 4);
    start_thread_pool();

    auto t1 = std::chrono::system_clock::now();

    start_gen_thread();
    init_chunks();
    auto t2 = std::chrono::system_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    Logger::info("Chunk Block Init Finish, Time Consuming: {}", d);

    start_server_thread();
    m_init = true;
}

void ServerWorld::init_chunks() { hot_reload(); }

void ServerWorld::gen_chunks_internal(const std::string& uuid) {
    // Logger::info("gen_chunks_internal");
    m_chunk_gen_finished = false;

    ChunkPosSet required_chunks_set;
    compute_required_chunks(required_chunks_set, uuid);
    std::vector<ChunkPos> need_gen_chunks_pos;

    ChunkPosSet old_set;
    sync_and_collect_missing_chunks(need_gen_chunks_pos, required_chunks_set);
    {
        std::lock_guard lock(m_player_mutex);
        auto it = m_players.find(uuid);
        if (it == m_players.end()) {
            return;
        }
        old_set = std::move(it->second.get_chunk_pos_set());
        it->second.update_chunk_set(required_chunks_set);
    }

    update_ref_count(old_set, required_chunks_set);
    ASSERT_MSG(!required_chunks_set.empty(), "required chunks is empty!!");

    Logger::info("New Gen Chunks Sum: {}", need_gen_chunks_pos.size());

    if (need_gen_chunks_pos.empty() && m_new_chunks.empty()) {
        m_could_gen = true;

        return;
    }
    {
        // Create new chunk
        std::lock_guard lock(m_new_chunk_mutex);
        for (auto& pos : need_gen_chunks_pos) {
            m_new_chunks.emplace(
                pos, std::make_unique<ServerChunk>(ServerChunk(*this, pos)));
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
    {
        std::lock_guard lock(m_chunks_mutex);
        for (auto pos : required_chunks) {
            auto it = m_chunks.find(pos);
            if (it == m_chunks.end()) {
                need_gen_chunks_pos.push_back(pos);
                m_chunks.emplace(
                    pos, ChunkEntity{ChunkState::GENERATING, nullptr, 0});
            }
        }
    }
}

void ServerWorld::submit_new_chunks(const std::string& uuid) {
    using enum ChunkLoadStyle;
    std::lock_guard lock(m_new_chunk_mutex);
    auto pool_ptr = m_gen_thread_pool.load();
    if (!pool_ptr) {
        return;
    }
    switch (m_chunk_load_style) {
    case RANDOM:
        // Enqueue directly in random order
        for (auto& [pos, task] : m_new_chunks) {
            if (!task.future.valid()) {
                task.future =
                    pool_ptr->enqueue([&task]() { task.chunk->gen_chunk(); });
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
        glm::vec3 player_pos = get_player_pos(uuid);

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
                    pool_ptr->enqueue([task]() { task->chunk->gen_chunk(); });
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
            try {
                pending.future.get();
            } catch (const std::exception& e) {
                Logger::error("Chunk generation failed: {}", e.what());
                return true;
            }
            // Spawn complete, move away
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
            std::string uuid;
            if (!m_need_gen_queue.empty()) {
                uuid = m_need_gen_queue.front();
                m_need_gen_queue.pop();
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
    if (m_gen_pool_threads == 0) {
        m_gen_pool_threads = change_pool_threads(m_gen_thread_pool,
                                                 max_thread - RESERVED_THREADS);
    } else {
        m_gen_pool_threads =
            change_pool_threads(m_gen_thread_pool, m_gen_pool_threads);
    }

    if (m_net_pool_threads == 0) {
        m_net_pool_threads = change_pool_threads(m_net_thread_pool, 4);
    } else {
        m_net_pool_threads =
            change_pool_threads(m_net_thread_pool, m_net_pool_threads);
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
    Logger::info("Gen Thread Pool Stopped");

    auto p = m_net_thread_pool.load();
    if (p) {
        p->stop();
    }
    m_net_thread_pool.store(nullptr);
    Logger::info("Net Thread Pool Stopped");
}

void ServerWorld::serever_run(std::stop_token stoken) {
    Logger::info("Server Thread Started!");

    using Clock = std::chrono::steady_clock;
    constexpr auto TICK = std::chrono::milliseconds(DEFAULT_PER_TICK_TIME);

    auto next = Clock::now();
    while (!stoken.stop_requested()) {
        next += TICK;
        if (m_tick_running) {
            ++m_game_ticks;
            m_day_tick = (m_day_tick + 1) % DAY_TIME;
        }
        update();
        std::this_thread::sleep_until(next);
    }
    Logger::info("Server Thread Stopped!");
}

void ServerWorld::need_gen(std::string uuid) {

    // if (!m_could_gen) {
    //     Logger::warn("It is generating or consuming new chunks");
    //     return;
    // }

    m_could_gen = false;

    {
        std::lock_guard lock(m_need_gen_queue_mutex);
        m_need_gen_queue.enqueue(std::move(uuid));
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
    if (it->second.state != ChunkState::READY) {
        return false;
    }
    auto [x, y, z] = ServerChunk::world_to_block(world_x, world_y, world_z,
                                                 chunk_x, chunk_z);
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return false;
    }

    it->second.chunk->set_chunk_block(ServerChunk::index(x, y, z), id);
    return true;
}

void ServerWorld::hot_reload() {
    auto& config = Config::get();
    int dist = config.get<int>("world.rendering_distance");
    m_rendering_distance = dist <= MAX_DISTANCE ? dist : MAX_DISTANCE;
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
    Arena arena;
    auto* rsp = Arena::Create<S2C_ClearAllChunks>(&arena);
    rsp->set_clear(true);
    {
        std::lock_guard lock(m_player_mutex);
        for (auto& [uuid, player] : m_players) {
            player.get_session()->send(make_packet(*rsp));
        }
    }

    m_is_rebuilding = false;
}

void ServerWorld::update() {
    poll_finished_chunks();
    {
        std::lock_guard lk(m_chunks_mutex);
        bool consumed = false;

        for (auto& x : m_new_finished_chunk) {
            auto it = m_chunks.find(x.pos);
            if (it == m_chunks.end()) {
                Logger::error(
                    "New Chunk {} {} not Find, don't move to m_chunks", x.pos.x,
                    x.pos.z);
                continue;
            }
            it->second.chunk = std::move(x.chunk);
            it->second.state = ChunkState::READY;
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
    std::string name;
    {
        std::lock_guard lock(m_player_mutex);
        auto it = m_players.find(uuid);
        if (it == m_players.end()) {
            Logger::warn("Player {} is not in this Server", uuid);
            return;
        }
        it->second.update_pos(x, y, z);
        it->second.update_sync_gametick(m_game_ticks);
        name = it->second.get_name();
    }

    // update other player pos;
    std::vector<std::shared_ptr<Session>> other;
    {
        std::shared_lock lock(m_player_mutex);
        for (auto& [o_uuid, player] : m_players) {
            if (o_uuid == uuid) {
                continue;
            }
            other.emplace_back(player.get_session());
        }
    }

    for (auto& session : other) {
        Arena arena;
        auto* rsp = Arena::Create<PlayerInfoRsp>(&arena);
        rsp->set_uuid(uuid);
        rsp->set_name(name);
        auto* pos = rsp->mutable_pos();
        pos->set_x(x);
        pos->set_y(y);
        pos->set_z(z);
        session->send(make_packet(*rsp));
    }
}

void ServerWorld::handle_player_login(const std::string& name,
                                      std::shared_ptr<Session> session) {
    std::string uuid = generate_uuid();
    Logger::info("Player {} (uuid {}) join the world", name, uuid);
    bool sucess = true;
    {
        std::lock_guard lock(m_player_mutex);
        auto [_, inserted] = m_players.emplace(
            std::piecewise_construct, std::forward_as_tuple(std::string(uuid)),
            std::forward_as_tuple(name, uuid, *this, session, m_game_ticks));
        if (!inserted) {
            Logger::error("Player insert Fail");
        }
        sucess = inserted;
    }

    Arena arena;
    if (!sucess) {
        auto* rsp = Arena::Create<LoginRsp>(&arena);
        rsp->set_success(false);
        session->send(make_packet(*rsp));
        return;
    }

    m_uuid_to_name.emplace(uuid, name);
    // Pre-insert into new_chunks to ensure correct addition to waiting_player
    /*ChunkPosSet required_chunks;
    compute_required_chunks(required_chunks, uuid);
    std::vector<ChunkPos> need_gen_chunks_pos;

    sync_and_collect_missing_chunks(need_gen_chunks_pos, required_chunks);

    {
        std::lock_guard lock(m_new_chunk_mutex);
        for (auto& pos : need_gen_chunks_pos) {
            m_new_chunks.emplace(pos, ServerChunk(*this, pos));
        }
    }
    */
    need_gen(uuid);

    auto* rsp = Arena::Create<LoginRsp>(&arena);
    rsp->set_success(true);
    rsp->set_uuid(uuid);
    session->send(make_packet(*rsp));
}

void ServerWorld::handle_player_exit(const std::string& uuid) {
    std::shared_ptr<Session> exit_session;
    ChunkPosSet old_set;
    {
        std::lock_guard lock(m_player_mutex);
        auto it = m_players.find(uuid);
        if (it != m_players.end()) {
            Logger::info("Player {} Exit the Server", it->second.get_name());
            exit_session = it->second.get_session();
            old_set = std::move(it->second.get_chunk_pos_set());
            m_players.erase(it);
        } else {
            Logger::error("Player {} isn't in Server", uuid);
            return;
        }
    }

    m_uuid_to_name.erase(uuid);

    update_ref_count(old_set, {});

    Arena arena;
    auto* rsp = Arena::Create<LogoutRsp>(&arena);
    rsp->set_uuid(uuid);
    rsp->set_server_stop(false);
    exit_session->send(make_packet(*rsp));

    std::vector<std::shared_ptr<Session>> sessions;
    {
        std::shared_lock lock(m_player_mutex);
        for (auto& [uuid, player] : m_players) {
            sessions.emplace_back(player.get_session());
        }
    }

    for (auto& s : sessions) {
        s->send(make_packet(*rsp));
    }
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

void ServerWorld::handle_chunk_req(int task_id, const std::string& uuid,
                                   ChunkPos pos) {
    {
        std::shared_lock lock(m_player_mutex);
        auto it = m_players.find(uuid);
        if (it == m_players.end()) {
            return;
        }
        if (it->second.task_id() < task_id) {
            // task_id is an atomic variable, can be operated on directly
            it->second.task_id(task_id);
        }
    }
    auto pool = m_net_thread_pool.load();
    pool->enqueue(
        [task_id, uuid, pos, this]() { send_chunk(task_id, uuid, pos); });
}

void ServerWorld::handle_block_change(const BlockChangeReq& req) {
    float x = req.pos().x();
    float y = req.pos().y();
    float z = req.pos().z();
    if (!set_block(glm::ivec3(x, y, z), req.block())) {
        return;
    }

    Arena arena;
    BlockChangeRsp* rsp = Arena::Create<BlockChangeRsp>(&arena);
    auto* pos = rsp->mutable_pos();
    pos->set_x(x);
    pos->set_y(y);
    pos->set_z(z);
    rsp->set_block(req.block());
    {
        std::shared_lock lock(m_player_mutex);
        for (auto& [uuid, player] : m_players) {
            auto session = player.get_session();
            if (session) {
                session->send(make_packet(*rsp));
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
int ServerWorld::gen_pool_threads() const { return m_gen_pool_threads.load(); }
int ServerWorld::max_threads() const { return m_max_threads.load(); }

void ServerWorld::change_pool_threads(ThreadPoolKind kind, int threads) {
    switch (kind) {
    case ThreadPoolKind::NET:
        m_net_pool_threads = change_pool_threads(m_net_thread_pool, threads);
        break;
    case ThreadPoolKind::GEN:
        m_gen_pool_threads = change_pool_threads(m_gen_thread_pool, threads);
        break;
    }
}

int ServerWorld::change_pool_threads(
    std::atomic<std::shared_ptr<ThreadPool>>& thread_pool, int threads) {
    m_max_threads = std::thread::hardware_concurrency();
    if (m_max_threads < 1) {
        Logger::warn("Can't Get Max Support Threads, Set Max Threads to 4");
        m_max_threads = 1;
    }
    int used_thread = std::clamp(threads, 1, m_max_threads.load());
    Logger::info("Create New Thread Pool Use {} Threads", used_thread);
    thread_pool.store(std::make_shared<ThreadPool>(used_thread));
    return used_thread;
}

void ServerWorld::send_server_stop() {
    Arena arena;
    auto* rsp = Arena::Create<LogoutRsp>(&arena);
    rsp->set_server_stop(true);
    std::shared_lock lock(m_player_mutex);
    for (auto& [uuid, player] : m_players) {
        player.get_session()->send(make_packet(*rsp));
    }
    Logger::info("Send Server Mesaage Success");
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

int ServerWorld::chunk_size() const {
    std::shared_lock lock(m_chunks_mutex);
    return m_chunks.size();
}

} // namespace Cubed