#pragma once

#include "Cubed/gameplay/cave_carver.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/game_time.hpp"
#include "Cubed/gameplay/river_worm.hpp"
#include "Cubed/gameplay/server_chunk.hpp"
#include "Cubed/gameplay/server_player.hpp"
#include "Cubed/tools/recent_queue.hpp"
#include "Cubed/tools/thread_pool.hpp"
#include "world/block_change.pb.h"

#include <future>
#include <shared_mutex>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
namespace Cubed {
class Session;
class ServerWorld {
public:
    enum class ThreadPoolKind { NET, GEN };
    ServerWorld();
    ~ServerWorld();
    void stop();
    void handle_player_exit(const std::string& uuid);
    void init_world();
    void need_gen(std::string uuid);
    void update();
    void hot_reload();

    void rebuild_world();
    int rendering_distance() const;
    void rendering_distance(int rendering_distance);
    void start_gen_thread();
    void start_server_thread();

    void stop_gen_thread();
    void stop_server_thread();

    void stop_thread_pool();
    void start_thread_pool();

    void serever_run(std::stop_token stoken);

    CaveCarver& cave_carcer();
    RiverWorm& river_worm();

    TickType game_tick() const;
    TickType day_tick() const;

    void day_tick(TickType tick);

    int per_tick_time() const;
    void per_tick_time(int ms);
    bool is_tick_running() const;
    void tick_running(bool run);

    int gen_pool_threads() const;
    int max_threads() const;

    void change_pool_threads(ThreadPoolKind kind, int threads);

    int chunk_load_style() const;
    void set_chunk_load_style(int id);

    bool set_block(const glm::ivec3& block_pos, unsigned id);

    void sync_player_pos(const std::string& uuid, float x, float y, float z);
    void handle_player_login(const std::string& player_name,
                             std::shared_ptr<Session> session);
    glm::vec3 get_player_pos(const std::string& uuid) const;

    void handle_chunk_req(int task_id, const std::string& uuid, ChunkPos pos);
    void handle_block_change(const BlockChangeReq& req);

    int chunk_size() const;
    template <typename Fn>
    void register_timer(std::string_view id, TickType threshold, Fn&& f) {
        m_timers.emplace(std::piecewise_construct,
                         std::forward_as_tuple(std::string(id)),
                         std::forward_as_tuple(threshold, std::forward<Fn>(f)));
    }

private:
    enum class ChunkState { NONE, GENERATING, READY, PENDING_DELETE };
    struct ChunkEntity {
        ChunkState state;
        std::shared_ptr<ServerChunk> chunk;
    };

    enum class ChunkLoadStyle { RANDOM, CENTER };
    struct PendingRequest {
        std::string uuid;
        int task_id;
        ChunkPos pos;
    };
    struct PendingChunk {
        std::unique_ptr<ServerChunk> chunk;
        std::future<void> future;
    };
    struct FinishedChunk {
        ChunkPos pos;
        std::unique_ptr<ServerChunk> chunk;
    };

    using ChunkHashMap =
        std::unordered_map<ChunkPos, ChunkEntity, ChunkPos::Hash>;
    using PlayerHashMap = std::unordered_map<std::string, ServerPlayer>;
    using PendingChunkHashMap =
        std::unordered_map<ChunkPos, PendingChunk, ChunkPos::Hash>;
    using ChunkPosSet = std::unordered_set<ChunkPos, ChunkPos::Hash>;
    using PlayerUUIDMap = tbb::concurrent_hash_map<std::string, std::string>;

    using uuid_acc = PlayerUUIDMap::accessor;
    using uuid_cacc = PlayerUUIDMap::const_accessor;
    // key = uuid
    PlayerHashMap m_players;
    ChunkHashMap m_chunks;
    PendingChunkHashMap m_new_chunks;
    std::vector<FinishedChunk> m_new_finished_chunk;

    CaveCarver m_cave_carcer;
    RiverWorm m_river_worm;

    std::jthread m_gen_thread;
    std::jthread m_server_thread;

    std::atomic<bool> m_chunk_gen_finished{false};
    std::atomic<bool> m_could_gen{true};
    std::atomic<bool> m_gen_running{false};
    std::atomic<bool> m_need_gen_chunk{false};
    std::atomic<bool> m_is_rebuilding{false};
    std::atomic<bool> m_init{false};
    std::atomic<bool> m_stopped{false};
    std::atomic<int> m_rendering_distance{24};
    std::atomic<int> m_gen_pool_threads{0};
    std::atomic<int> m_net_pool_threads{0};
    std::atomic<int> m_max_threads{1};

    std::atomic<TickType> m_game_ticks{0};
    std::atomic<TickType> m_day_tick{6000};
    std::atomic<bool> m_tick_running{true};
    std::atomic<int> m_per_tick_time = DEFAULT_PER_TICK_TIME; // ms

    mutable std::shared_mutex m_chunks_mutex;
    std::shared_mutex m_new_chunk_mutex;
    mutable std::shared_mutex m_player_mutex;
    std::mutex m_need_gen_queue_mutex;
    std::condition_variable_any m_gen_cv;

    RecentQueue<std::string> m_need_gen_queue;

    std::atomic<std::shared_ptr<ThreadPool>> m_gen_thread_pool;
    std::atomic<std::shared_ptr<ThreadPool>> m_net_thread_pool;

    std::atomic<ChunkLoadStyle> m_chunk_load_style{ChunkLoadStyle::CENTER};

    PlayerUUIDMap m_uuid_to_name;

    tbb::concurrent_unordered_map<std::string, Timer> m_timers;
    tbb::concurrent_queue<PendingRequest> m_waiting_chunk_requests;

    void init_chunks();

    void gen_chunks_internal(const std::string& uuid);

    void compute_required_chunks(ChunkPosSet& required_chunks,
                                 const std::optional<std::string>& uuid);
    void sync_and_collect_missing_chunks(std::vector<ChunkPos>&,
                                         const ChunkPosSet&);
    void submit_new_chunks(const std::string& uuid);
    void poll_finished_chunks();
    void wait_all_chunk_tasks();

    void clear_unused_chunks();

    void send_time();

    void send_chunk(int task_id, const std::string& uuid, ChunkPos pos);

    int
    change_pool_threads(std::atomic<std::shared_ptr<ThreadPool>>& thread_pool,
                        int threads);
    void send_server_stop();
};
} // namespace Cubed
