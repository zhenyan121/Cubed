#include "Cubed/gameplay/client_world.hpp"

#include "Cubed/config.hpp"
#include "Cubed/gameplay/game_time.hpp"
#include "Cubed/gameplay/packet.hpp"

#include <numbers>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace google::protobuf;
namespace Cubed {

namespace {
struct ChunkRenderData {
    std::array<const std::vector<BlockType>*, 4> neighbor_block;
    ClientChunk* chunk;
};
} // namespace

ClientWorld::ClientWorld() : m_player(*this) {}

ClientWorld::~ClientWorld() {
    stop_client_thread();
    stop_thread_pool();
    {
        std::lock_guard lock(m_chunks_mutex);
        m_chunks.clear();
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
    m_timers.clear();
}

const std::optional<LookBlock>&
ClientWorld::get_look_block_pos(const std::string& name) const {

    return m_player.get_look_block_pos();
}

ClientPlayer& ClientWorld::get_player() { return m_player; }

int ClientWorld::get_block(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = get_chunk_pos(block_pos.x, block_pos.z);
    std::shared_lock lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return 0;
    }

    const auto& chunk_blocks = it->second.get_chunk_blocks();
    auto [x, y, z] = ClientChunk::world_to_block(block_pos, {chunk_x, chunk_z});
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return 0;
    }
    return chunk_blocks[ClientChunk::index(x, y, z)];
}
bool ClientWorld::is_solid(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = get_chunk_pos(block_pos.x, block_pos.z);
    std::shared_lock lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return false;
    }
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    auto [x, y, z] = ClientChunk::world_to_block(block_pos, {chunk_x, chunk_z});
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return false;
    }
    auto id = chunk_blocks[ClientChunk::index(x, y, z)];
    if (BlockManager::is_gas(id) || BlockManager::is_liquid(id)) {
        return false;
    } else {
        return true;
    }
}
bool ClientWorld::can_pass_block(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = get_chunk_pos(block_pos.x, block_pos.z);
    std::shared_lock lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return true;
    }
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    auto [x, y, z] = ClientChunk::world_to_block(block_pos, {chunk_x, chunk_z});
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return true;
    }
    auto id = chunk_blocks[ClientChunk::index(x, y, z)];
    return BlockManager::is_passable(id);
}

void ClientWorld::rebuild_world() {
    if (m_is_rebuilding.exchange(true)) {
        return;
    }
    stop_thread_pool();
    {
        std::lock_guard lk(m_chunks_mutex);
        m_chunks.clear();
    }
    {
        std::lock_guard lock(m_pending_upload_queue_mutex);
        m_pending_upload_queue.clear();
    }
    start_thread_pool();
    request_chunk();
    m_is_rebuilding = false;
}

BlockType ClientWorld::get_block_tpye(const glm::ivec3& block_pos) const {
    auto [chunk_x, chunk_z] = get_chunk_pos(block_pos.x, block_pos.z);
    std::shared_lock lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        // Logger::error("Can't Find Block {} {} {}", block_pos.x, block_pos.y,
        //               block_pos.z);
        return 0;
    }
    const auto& chunk_blocks = it->second.get_chunk_blocks();
    auto [x, y, z] = ClientChunk::world_to_block(block_pos, {chunk_x, chunk_z});
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        // Logger::error("Can't Find Block {} {} {}", block_pos.x, block_pos.y,
        //               block_pos.z);
        return 0;
    }
    return chunk_blocks[ClientChunk::index(x, y, z)];
}
void ClientWorld::set_block(const glm::ivec3& block_pos, unsigned id) {
    int world_x, world_y, world_z;
    world_x = block_pos.x;
    world_y = block_pos.y;
    world_z = block_pos.z;

    auto [chunk_x, chunk_z] = get_chunk_pos(world_x, world_z);
    std::lock_guard lk(m_chunks_mutex);
    auto it = m_chunks.find(ChunkPos{chunk_x, chunk_z});

    if (it == m_chunks.end()) {
        return;
    }

    auto [x, y, z] = ClientChunk::world_to_block(world_x, world_y, world_z,
                                                 chunk_x, chunk_z);
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
        z >= CHUNK_SIZE) {
        return;
    }

    it->second.set_chunk_block(ClientChunk::index(x, y, z), id);

    static const glm::ivec3 NEIGHBOR_DIRS[] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, 0, 1}};

    for (const auto& dir : NEIGHBOR_DIRS) {
        glm::ivec3 neighbor = block_pos + dir;

        auto [cx, cz] = get_chunk_pos(neighbor.x, neighbor.z);
        auto it = m_chunks.find({cx, cz});
        if (it != m_chunks.end()) {
            it->second.mark_dirty();
        }
    }
}
void ClientWorld::push_delete_vbo(GLuint vbo) {
    std::lock_guard lk(m_delete_vbo_mutex);
    m_pending_delete_vbo.push_back(vbo);
}
void ClientWorld::push_delete_vao(GLuint vao) {
    std::lock_guard lk(m_delete_vao_mutex);
    m_pending_delete_vao.push_back(vao);
}

void ClientWorld::report_block_change(const glm::ivec3& pos,
                                      unsigned id) const {
    Arena arena;
    auto* req = Arena::Create<BlockChangeReq>(&arena);
    req->set_uuid(m_player.get_uuid());
    req->set_block(id);
    auto* p = req->mutable_pos();
    p->set_x(pos.x);
    p->set_y(pos.y);
    p->set_z(pos.z);
    m_client->send(make_packet(*req));
}

void ClientWorld::receive_block_change(const BlockChangeRsp& rsp) {
    glm::vec3 pos{rsp.pos().x(), rsp.pos().y(), rsp.pos().z()};
    set_block(pos, rsp.block());
}

void ClientWorld::receive_time(const UpdateTime& rsp) {
    m_game_ticks = rsp.game_tick();
    m_day_tick = rsp.day_tick();
}

void ClientWorld::receive_remote_player(const PlayerInfoRsp& rsp) {
    {
        std::lock_guard lock(m_other_players_mutex);
        glm::vec3 pos{rsp.pos().x(), rsp.pos().y(), rsp.pos().z()};
        auto it = m_other_players.find(rsp.uuid());
        if (it == m_other_players.end()) {
            m_other_players.emplace(
                std::piecewise_construct, std::forward_as_tuple(rsp.uuid()),
                std::forward_as_tuple(rsp.name(), pos, pos));
        } else {
            it->second.target_pos = pos;
        }
        // Logger::info("Player {} pos Update", rsp.name());
    }
}

void ClientWorld::receive_player_logout(const LogoutRsp& rsp) {
    {
        std::lock_guard lock(m_other_players_mutex);
        int sum = m_other_players.erase(rsp.uuid());
        if (sum == 0) {
            Logger::warn("Player {} not find", rsp.uuid());
        } else {
            Logger::info("Player {} erase", rsp.uuid());
        }
    }
    if (rsp.uuid() == m_player.get_uuid()) {
        m_receive_exit = true;
    }
}

void ClientWorld::init(std::string_view player_name,
                       std::shared_ptr<NetworkClient> client) {
    m_chunks.reserve(MAX_DISTANCE * MAX_DISTANCE * 4);
    m_player.init(player_name);
    m_client = client;
    // timer
    register_timer("player_pos", 1, [this]() { report_player_pos(); });
    LoginReq req;
    req.set_name(m_player.get_name());
    while (!client->is_connected()) {
        if (client->is_connect_error()) {
            throw std::runtime_error("Can't connect to the server");
        }
        std::this_thread::sleep_for(milliseconds(200));
    }
    start_thread_pool();
    // request login
    Logger::info("Send Login Request");
    m_client->send(make_packet(req));
}

void ClientWorld::start_client_thread(std::string_view uuid) {
    if (m_game_running) {
        Logger::error("Game Already Running");
        return;
    }
    // response
    m_player.set_uuid(uuid);
    m_client_thread = std::jthread([this](std::stop_token token) {
        m_game_running = true;
        client_run(token);
    });
    request_chunk();
}

void ClientWorld::stop_client_thread() {
    m_client_thread.request_stop();
    if (m_client_thread.joinable()) {
        m_client_thread.join();
    }
    m_game_running = false;
}
void ClientWorld::start_thread_pool() {
    int max_threads = std::thread::hardware_concurrency();
    int threads = std::min<size_t>(max_threads, 4);
    change_pool_threads(threads);
}
void ClientWorld::stop_thread_pool() {
    auto pool_ptr = m_thread_pool.load();
    if (pool_ptr) {
        pool_ptr->stop();
    }
    m_thread_pool.store(nullptr);
    Logger::info("Thread Pool Stopped");
}

void ClientWorld::change_pool_threads(int threads) {
    int m_max_threads = std::thread::hardware_concurrency();
    if (m_max_threads < 1) {
        Logger::warn("Can't Get Max Support Threads, Set Max Threads to 4");
        m_max_threads = 1;
    }
    int used_thread = std::clamp(threads, 1, m_max_threads);
    Logger::info("Create New Thread Pool Use {} Threads", used_thread);
    m_thread_pool.store(std::make_shared<ThreadPool>(used_thread));
}

void ClientWorld::hot_reload() {
    auto& config = Config::get();
    int dist = config.get<int>("world.rendering_distance");
    Logger::info("Get Config Randering dist {}", dist);
    m_rendering_distance = dist <= MAX_DISTANCE ? dist : MAX_DISTANCE;
    request_chunk();
}

void ClientWorld::client_run(std::stop_token stoken) {
    Logger::info("Client Thread Started");
    using Clock = std::chrono::steady_clock;

    constexpr auto TICK = std::chrono::milliseconds(DEFAULT_PER_TICK_TIME);

    auto next = Clock::now();
    while (!stoken.stop_requested()) {
        next += TICK;
        for (auto& x : m_timers) {
            x.second.update();
        }
        std::this_thread::sleep_until(next);
    }
}

void ClientWorld::report_player_pos() {
    if (!m_client) {
        return;
    }
    Arena arena;
    auto* pos = Arena::Create<PlayerPos>(&arena);
    pos->set_uuid(m_player.get_uuid());
    glm::vec3 player_pos = m_player.get_player_pos();
    auto* v3 = pos->mutable_pos();
    v3->set_x(player_pos.x);
    v3->set_y(player_pos.y);
    v3->set_z(player_pos.z);
    m_client->send(make_packet(*pos));
}

void ClientWorld::request_chunk() {
    if (m_requesting_chunk.exchange(true)) {
        Logger::warn("It is requesting new chunk!");
        return;
    }
    ChunkPosSet required_chunks;

    glm::vec3 player_pos = m_player.get_player_pos();

    int x = std::floor(player_pos.x);
    int z = std::floor(player_pos.z);
    auto [chunk_x, chunk_z] = get_chunk_pos(x, z);
    int radius = m_rendering_distance;
    Logger::info("Client Chunk Radius {}", radius);
    int r2 = radius * radius;
    required_chunks.reserve(radius * radius);

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            if (dx * dx + dz * dz <= r2) {
                required_chunks.emplace(chunk_x + dx, chunk_z + dz);
            }
        }
    }

    ChunkPosVector need_send_pos;
    {
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
                need_send_pos.emplace_back(pos);
            }
        }
    }
    if (need_send_pos.empty()) {
        m_requesting_chunk = false;
        return;
    }
    using enum ChunkLoadStyle;
    switch (m_chunk_load_style) {
    case RANDOM:

        break;
    case CENTER: {

        glm::vec3 player_pos = m_player.get_player_pos();
        auto dist2 = [player_pos](ChunkPos chunk_pos) {
            ChunkPos player_chunk_pos =
                get_chunk_pos(player_pos.x, player_pos.z);
            float dx = player_chunk_pos.x - chunk_pos.x;
            float dz = player_chunk_pos.z - chunk_pos.z;
            return dx * dx + dz * dz;
        };

        std::sort(need_send_pos.begin(), need_send_pos.end(),
                  [&dist2](const auto& a, const auto& b) {
                      return dist2(a) < dist2(b);
                  });
    }
    }
    auto uuid = m_player.get_uuid();
    Arena arena;
    ++m_chunk_task_id;
    auto* req = Arena::Create<ChunkDataReq>(&arena);
    for (const auto& pos : need_send_pos) {
        req->set_task_id(m_chunk_task_id.load());
        req->set_uuid(uuid);
        auto* p = req->mutable_pos();
        p->set_x(pos.x);
        p->set_z(pos.z);
        m_client->send(make_packet(*req));
    }
    Logger::info("Send Chunk Request Success");
    m_requesting_chunk = false;
}

void ClientWorld::receive_chunk(ChunkDataRsp data) {
    if (data.task_id() < m_chunk_task_id) {
        return;
    }

    {
        std::lock_guard lock(m_chunks_mutex);
        ChunkPos pos{data.pos().x(), data.pos().z()};
        if (m_chunks.find(pos) != m_chunks.end()) {
            Logger::warn("Chunk {} {} has already in client world", pos.x,
                         pos.z);
            return;
        }
    }
    // vertex data will genrator in  client thread pool instead of net thread;
    auto pool = m_thread_pool.load();
    if (!pool) {
        Logger::error("Client Thread Pool is nullptr");
        return;
    }
    pool->enqueue([this, data = std::move(data)]() {
        ClientChunk chunk{*this};
        chunk.receive_chunk(data);
        {
            std::lock_guard lock(m_pending_upload_queue_mutex);
            m_pending_upload_queue.emplace_back(std::move(chunk));
        }
    });
}
void ClientWorld::receive_exit() { m_receive_exit = true; }

void ClientWorld::request_exit() {
    if (m_receive_exit) {
        return;
    }
    Arena arena;
    auto* req = Arena::Create<LogoutReq>(&arena);
    req->set_uuid(m_player.get_uuid());
    m_client->send(make_packet(*req));
    int cnt = 0;
    while (!m_receive_exit) {
        std::this_thread::sleep_for(milliseconds(DEFAULT_PER_TICK_TIME));
        ++cnt;
        if (cnt >= WORLD_EXIT_TIMEOUT) {
            Logger::warn("Can't Receive Server Exit Sign");
            break;
        }
    }
}

void ClientWorld::update(float delta_time) {
    m_player.update(delta_time);
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
    std::vector<ClientChunk> new_chunks;
    {
        std::lock_guard lock(m_pending_upload_queue_mutex);
        for (auto& c : m_pending_upload_queue) {
            // Logger::info("{} {}", c.get_chunk_pos().x, c.get_chunk_pos().z);
            new_chunks.emplace_back(std::move(c));
        }
        m_pending_upload_queue.clear();
    }
    for (auto& c : new_chunks) {
        c.upload_to_gpu();
    }
    {
        std::lock_guard lock(m_chunks_mutex);
        for (auto& c : new_chunks) {
            m_chunks.emplace(c.get_chunk_pos(), std::move(c));
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
    m_render_player_data.clear();
    {
        std::lock_guard lock(m_other_players_mutex);
        for (auto& [uuid, player] : m_other_players) {
            player.render_pos =
                glm::mix(player.render_pos, player.target_pos, 0.15f);
            m_render_player_data.emplace_back(player.name, player.render_pos);
        }
    }
}

glm::vec3 ClientWorld::sunlight_dir() const {
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
int ClientWorld::rendering_distance() const {
    return m_rendering_distance.load();
}

void ClientWorld::rendering_distance(int rendering_distance) {
    m_rendering_distance = rendering_distance;
    Logger::info("Set Rendering dist {} , the value is {}", rendering_distance,
                 m_rendering_distance.load());
    request_chunk();
}

int ClientWorld::get_chunk_task_id() const { return m_chunk_task_id.load(); }

const std::vector<ChunkRenderSnapshot>& ClientWorld::render_snapshots() const {
    return m_render_snapshots;
};
const std::vector<RemotePlayerRenderData>&
ClientWorld::render_player_data() const {
    return m_render_player_data;
}
std::vector<glm::vec4>& ClientWorld::planes() { return m_planes; }
} // namespace Cubed