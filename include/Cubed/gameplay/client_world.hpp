#pragma once
#include "Cubed/gameplay/block.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/client_chunk.hpp"
#include "Cubed/gameplay/client_player.hpp"
#include "Cubed/gameplay/game_time.hpp"
#include "Cubed/gameplay/network_client.hpp"
#include "Cubed/tools/thread_pool.hpp"

#include <deque>
#include <tbb/concurrent_unordered_map.h>
namespace Cubed {

struct OtherPlayerInfo {
    std::string name;
    glm::vec3 pos;
};

struct RenderPlayerData {
    std::string name;
    glm::vec3 pos;
};

class ClientWorld {
public:
    ClientWorld();
    ~ClientWorld();
    void init(std::string_view player_name,
              std::shared_ptr<NetworkClient> client);
    void update(float delta_time);
    const std::optional<LookBlock>&
    get_look_block_pos(const std::string& name) const;
    ClientPlayer& get_player();
    int get_block(const glm::ivec3& block_pos) const;
    bool is_solid(const glm::ivec3& block_pos) const;
    bool can_pass_block(const glm::ivec3& block_pos) const;
    BlockType get_block_tpye(const glm::ivec3& block_pos) const;

    void push_delete_vbo(GLuint vbo);
    void push_delete_vao(GLuint vao);
    // void hot_reload();

    // void rebuild_world();
    void report_block_change(const glm::ivec3& pos, unsigned id) const;
    void receive_block_change(const BlockChangeRsp& rsp);
    void receive_time(const UpdateTime& rsp);

    void receive_other_player(const PlayerInfoRsp& rsp);
    void receive_player_logout(const LogoutRsp& rsp);
    int rendering_distance() const;
    void rendering_distance(int rendering_distance);
    void start_client_thread(std::string_view uuid);
    void stop_client_thread();

    void start_thread_pool();
    void stop_thread_pool();
    void change_pool_threads(int threads);
    void hot_reload();
    void request_chunk();
    std::vector<glm::vec4>& planes();
    const std::vector<ChunkRenderSnapshot>& render_snapshots() const;
    const std::vector<RenderPlayerData>& render_player_data() const;
    glm::vec3 sunlight_dir() const;
    void receive_chunk(const ChunkDataRsp& data);
    void exit();
    template <typename Fn>
    void register_timer(std::string_view id, TickType threshold, Fn&& f) {
        m_timers.emplace(std::piecewise_construct,
                         std::forward_as_tuple(std::string(id)),
                         std::forward_as_tuple(threshold, std::forward<Fn>(f)));
    }

private:
    enum class ChunkLoadStyle { RANDOM, CENTER };
    using ChunkHashMap =
        tbb::concurrent_unordered_map<ChunkPos, ClientChunk, ChunkPos::Hash>;
    using ChunkPosSet = std::unordered_set<ChunkPos, ChunkPos::Hash>;
    using ChunkPosVector = std::vector<ChunkPos>;
    using OtherPlayerHashMap = std::unordered_map<std::string, OtherPlayerInfo>;
    ClientPlayer m_player;
    OtherPlayerHashMap m_other_players;
    ChunkHashMap m_chunks;
    std::vector<glm::vec4> m_planes;
    std::jthread m_client_thread;

    mutable std::shared_mutex m_chunks_mutex;
    std::mutex m_delete_vbo_mutex;
    std::mutex m_delete_vao_mutex;
    std::mutex m_pending_upload_queue_mutex;
    std::mutex m_other_players_mutex;

    std::deque<ClientChunk> m_pending_upload_queue;

    std::vector<GLuint> m_pending_delete_vbo;
    std::vector<GLuint> m_pending_delete_vao;

    std::deque<ChunkPos> m_dirty_queue;
    std::vector<ChunkRenderSnapshot> m_render_snapshots;
    std::vector<RenderPlayerData> m_render_player_data;
    tbb::concurrent_unordered_map<std::string, Timer> m_timers;
    std::atomic<bool> m_game_running{false};
    std::atomic<int> m_rendering_distance{24};
    std::atomic<TickType> m_game_ticks{0};
    std::atomic<TickType> m_day_tick{6000};
    std::atomic<bool> m_requesting_chunk{false};
    std::shared_ptr<NetworkClient> m_client;
    ChunkLoadStyle m_chunk_load_style{ChunkLoadStyle::CENTER};

    std::atomic<std::shared_ptr<ThreadPool>> m_thread_pool;

    void client_run(std::stop_token token);

    void set_player_pos();

    void report_player_pos();

    void set_block(const glm::ivec3& pos, unsigned id);
};
} // namespace Cubed
