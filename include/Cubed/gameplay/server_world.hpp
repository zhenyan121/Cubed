#pragma once

#include "Cubed/gameplay/cave_carver.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/game_time.hpp"
#include "Cubed/gameplay/river_worm.hpp"
#include "Cubed/gameplay/server_chunk.hpp"
#include "Cubed/gameplay/server_player.hpp"
#include "Cubed/tools/thread_pool.hpp"
#include "world/block_change.pb.h"

#include <future>
#include <shared_mutex>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_unordered_map.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
namespace Cubed {
class Session;
class ServerWorld {
public:
    ServerWorld();
    ~ServerWorld();
    void player_join(std::string_view name, std::string_view uuid);
    void player_exit(const std::string& name);
    void init_world();
    void need_gen(std::optional<std::string> uuid);
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

    int pool_threads() const;
    int max_threads() const;
    void change_pool_threads(int threads);

    int chunk_load_style() const;
    void set_chunk_load_style(int id);

    bool set_block(const glm::ivec3& block_pos, unsigned id);

    void sync_player_pos(const std::string& uuid, float x, float y, float z);
    void handle_player_login(const std::string& player_name,
                             std::shared_ptr<Session> session);
    glm::vec3 get_player_pos(const std::string& uuid) const;

    void handle_chunk_req(const std::string& uuid, ChunkPos pos);
    void handle_block_change(const BlockChangeReq& req);

private:
    enum class ChunkLoadStyle { RANDOM, CENTER };
    struct PendingChunk {
        ServerChunk chunk;
        std::future<void> future;
    };
    using ChunkHashMap =
        std::unordered_map<ChunkPos, ServerChunk, ChunkPos::Hash>;
    using PlayerHashMap =
        tbb::concurrent_unordered_map<std::string, ServerPlayer>;
    using PendingChunkHashMap =
        std::unordered_map<ChunkPos, PendingChunk, ChunkPos::Hash>;
    using ChunkPosSet = std::unordered_set<ChunkPos, ChunkPos::Hash>;
    using PlayerSessionMap =
        tbb::concurrent_hash_map<std::string, std::shared_ptr<Session>>;
    using session_acc = PlayerSessionMap::accessor;
    using session_cacc = PlayerSessionMap::const_accessor;
    PlayerHashMap m_players;
    ChunkHashMap m_chunks;
    // Can only be used in the gen thread
    PendingChunkHashMap new_chunks;
    std::vector<std::pair<ChunkPos, ServerChunk>> m_new_finished_chunk;

    CaveCarver m_cave_carcer;
    RiverWorm m_river_worm;

    std::jthread m_gen_thread;
    std::jthread m_server_thread;

    std::atomic<bool> m_chunk_gen_finished{false};
    std::atomic<bool> m_could_gen{true};
    std::atomic<bool> m_gen_running{false};
    std::atomic<bool> m_need_gen_chunk{false};
    std::atomic<bool> m_is_rebuilding{false};
    std::atomic<int> m_rendering_distance{24};
    std::atomic<int> m_pool_threads{0};
    std::atomic<int> m_max_threads{1};

    std::atomic<TickType> m_game_ticks{0};
    std::atomic<TickType> m_day_tick{6000};
    std::atomic<bool> m_tick_running{true};
    std::atomic<int> m_per_tick_time = DEFAULT_PER_TICK_TIME; // ms

    std::shared_mutex m_chunks_mutex;
    std::shared_mutex m_new_chunk_mutex;
    std::mutex m_need_gen_queue_mutex;
    std::condition_variable m_gen_cv;

    std::deque<std::string> m_need_gen_queue;

    std::atomic<std::shared_ptr<ThreadPool>> m_gen_thread_pool;

    std::atomic<ChunkLoadStyle> m_chunk_load_style{ChunkLoadStyle::RANDOM};

    // key = uuid
    PlayerSessionMap m_player_session;
    tbb::concurrent_hash_map<std::string, std::string> m_uuid_to_name;
    void init_chunks();

    void gen_chunks_internal(std::optional<std::string> uuid);

    void compute_required_chunks(ChunkPosSet& required_chunks,
                                 const std::optional<std::string>& uuid);
    void sync_and_collect_missing_chunks(std::vector<ChunkPos>&,
                                         const ChunkPosSet&);
    void submit_new_chunks(const std::optional<std::string>& uuid);
    void poll_finished_chunks();
    void wait_all_chunk_tasks();
};
} // namespace Cubed
