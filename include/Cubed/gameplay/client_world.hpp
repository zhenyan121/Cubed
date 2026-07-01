#pragma once
#include "Cubed/gameplay/block.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/client_chunk.hpp"
#include "Cubed/gameplay/client_player.hpp"
#include "Cubed/gameplay/game_time.hpp"
#include "Cubed/gameplay/network_client.hpp"
#include "Cubed/tools/priority_thread_pool.hpp"

#include <absl/container/flat_hash_set.h>
#include <deque>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
namespace Cubed {

struct RemotePlayerInfo {
    std::string name;
    glm::vec3 render_pos;
    glm::vec3 target_pos;
};

struct RemotePlayerRenderData {
    std::string name;
    glm::vec3 render_pos;
};

class ClientWorld {
public:
    ClientWorld();
    ~ClientWorld();
    void init(std::string_view player_name,
              std::shared_ptr<NetworkClient> client);
    void update(float delta_time);
    const std::optional<LookBlock>& get_look_block_pos() const;
    ClientPlayer& get_player();
    int get_block(const glm::ivec3& block_pos) const;
    bool is_solid(const glm::ivec3& block_pos) const;
    bool can_pass_block(const glm::ivec3& block_pos) const;
    BlockType get_block_tpye(const glm::ivec3& block_pos) const;

    void rebuild_world();

    void push_delete_vbo(GLuint vbo);
    void push_delete_vao(GLuint vao);
    // void hot_reload();

    // void rebuild_world();
    void report_block_change(const glm::ivec3& pos, unsigned id) const;
    void receive_block_change(const BlockChangeRsp& rsp);
    void receive_time(const UpdateTime& rsp);

    void receive_remote_player(const PlayerInfoRsp& rsp);
    void receive_player_logout(const LogoutRsp& rsp);
    int rendering_distance() const;
    void rendering_distance(int rendering_distance);
    int get_chunk_task_id() const;
    void start_client_thread(std::string_view uuid);
    void stop_client_thread();

    void start_thread_pool();
    void stop_thread_pool();
    void change_pool_threads(int threads);
    void hot_reload();
    void request_chunk();
    std::vector<glm::vec4>& planes();
    const std::vector<const ChunkRenderSnapshot*>& render_snapshots() const;
    const std::vector<RemotePlayerRenderData>& render_player_data() const;
    glm::vec3 sunlight_dir() const;
    void receive_chunk(std::vector<uint8_t> data, PacketHeader header);
    void request_exit();
    bool is_receive_exit();
    int chunk_size() const;
    static AABB get_block_aabb(const glm::ivec3& pos);
    template <typename Fn>
    void register_timer(std::string_view id, TickType threshold, Fn&& f) {
        m_timers.emplace(std::piecewise_construct,
                         std::forward_as_tuple(std::string(id)),
                         std::forward_as_tuple(threshold, std::forward<Fn>(f)));
    }

private:
    enum class ChunkLoadStyle { RANDOM, CENTER };
    using ChunkHashMap =
        tbb::concurrent_hash_map<ChunkPos, std::shared_ptr<ClientChunk>,
                                 ChunkPos::TBBHash>;
    using ChunkPosSet = absl::flat_hash_set<ChunkPos, ChunkPos::Hash>;
    using ChunkPosVector = std::vector<ChunkPos>;
    using OtherPlayerHashMap =
        std::unordered_map<std::string, RemotePlayerInfo>;
    using chunk_acc = ChunkHashMap::accessor;
    using chunk_cacc = ChunkHashMap::const_accessor;
    static constexpr int WORLD_EXIT_TIMEOUT = 200;
    static constexpr int MAX_UPLOAD_CHUNK_SUM = 16;
    ClientPlayer m_player;
    OtherPlayerHashMap m_other_players;
    ChunkHashMap m_chunks;
    std::vector<glm::vec4> m_planes;
    std::jthread m_client_thread;

    std::mutex m_delete_vbo_mutex;
    std::mutex m_delete_vao_mutex;
    mutable std::shared_mutex m_other_players_mutex;

    tbb::concurrent_queue<std::unique_ptr<ClientChunk>> m_pending_upload_queue;
    tbb::concurrent_queue<ChunkPos> m_dirty_chunk_queue;

    std::vector<GLuint> m_pending_delete_vbo;
    std::vector<GLuint> m_pending_delete_vao;

    std::deque<ChunkPos> m_dirty_queue;
    std::vector<const ChunkRenderSnapshot*> m_render_snapshots;
    std::vector<RemotePlayerRenderData> m_render_player_data;
    tbb::concurrent_unordered_map<std::string, Timer> m_timers;
    std::atomic<bool> m_game_running{false};
    std::atomic<bool> m_receive_exit{false};
    std::atomic<int> m_rendering_distance{24};
    std::atomic<TickType> m_game_ticks{0};
    std::atomic<TickType> m_day_tick{6000};
    std::atomic<bool> m_requesting_chunk{false};
    std::atomic<bool> m_is_rebuilding{false};
    std::atomic<int> m_chunk_task_id{0};
    std::shared_ptr<NetworkClient> m_client;
    ChunkLoadStyle m_chunk_load_style{ChunkLoadStyle::CENTER};

    std::atomic<std::shared_ptr<PriorityThreadPool>> m_thread_pool;

    void client_run(std::stop_token token);

    void set_player_pos();

    void report_player_pos();

    void set_block(const glm::ivec3& pos, unsigned id);

    void update_chunk(const ChunkPosSet& old, const ChunkPosSet& now);
};
} // namespace Cubed
