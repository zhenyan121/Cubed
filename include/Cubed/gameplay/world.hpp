#pragma once
#include "Cubed/AABB.hpp"
#include "Cubed/gameplay/cave_carver.hpp"
#include "Cubed/gameplay/chunk.hpp"
#include "Cubed/gameplay/game_time.hpp"
#include "Cubed/gameplay/river_worm.hpp"
#include "Cubed/tools/thread_pool.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace Cubed {

struct ChunkRenderSnapshot {
    GLuint normal_vao;
    size_t normal_vertices_count;
    GLuint cross_vao;
    size_t cross_vertices_count;
    GLuint normal_discard_vao;
    size_t normal_discard_vertices_count;
    GLuint normal_blend_vao;
    size_t normal_blend_vertices_count;
    GLuint water_vao;
    size_t water_vertices_count;
    glm::vec3 center;
    glm::vec3 half_extents;
};

class Player;
class TextureManager;
class World {
private:
    struct PendingChunk {
        Chunk chunk;
        std::future<void> future;
    };

    using OptionalBlockVectorArray =
        std::array<std::optional<std::vector<BlockType>>, 4>;
    using ChunkPtrUpdateList = std::vector<std::pair<ChunkPos, Chunk*>>;
    using ChunkPairVector = std::vector<std::pair<ChunkPos, Chunk>>;
    using ChunkPairQueue = std::queue<std::pair<ChunkPos, Chunk>>;
    using ConstChunkMap =
        std::unordered_map<ChunkPos, const Chunk*, ChunkPos::Hash>;
    using ChunkPosSet = std::unordered_set<ChunkPos, ChunkPos::Hash>;
    using ChunkHashMap = std::unordered_map<ChunkPos, Chunk, ChunkPos::Hash>;
    using PendingChunkHashMap =
        std::unordered_map<ChunkPos, PendingChunk, ChunkPos::Hash>;
    glm::vec3 m_gen_player_pos{0.0f, 0.0f, 0.0f};
    ChunkHashMap m_chunks;
    std::unordered_map<std::size_t, Player> m_players;
    std::vector<glm::vec4> m_planes;

    std::thread m_gen_thread;
    std::thread m_server_thread;
    std::atomic<std::shared_ptr<ThreadPool>> m_gen_thread_pool;
    std::stop_source m_server_stop_source;

    std::atomic<int> m_per_tick_time = DEFAULT_PER_TICK_TIME; // ms

    std::atomic<TickType> m_day_tick = 6000;

    mutable std::mutex m_chunks_mutex;
    std::mutex m_gen_signal_mutex;
    std::mutex m_new_chunk_mutex;
    std::mutex m_delete_vbo_mutex;
    std::mutex m_delete_vao_mutex;
    std::mutex m_gen_player_pos_mutex;
    std::vector<GLuint> m_pending_delete_vbo;
    std::vector<GLuint> m_pending_delete_vao;
    std::condition_variable m_gen_cv;
    std::atomic<bool> m_gen_running{false};
    std::atomic<bool> m_need_gen_chunk{false};
    std::atomic<bool> m_is_rebuilding{false};
    std::atomic<bool> m_chunk_gen_finished{false};
    std::atomic<bool> m_could_gen{true};
    std::atomic<bool> m_tick_running{true};
    std::atomic<int> m_rendering_distance{24};
    std::atomic<float> m_chunk_gen_fraction{0.0f};

    std::atomic<TickType> m_game_ticks{0};

    std::vector<ChunkPos> m_dirty_queue;
    std::vector<ChunkRenderSnapshot> m_render_snapshots;
    std::vector<std::pair<ChunkPos, Chunk>> m_new_finished_chunk;
    // Can only be used in the gen thread
    PendingChunkHashMap new_chunks;

    CaveCarver m_cave_carcer;
    RiverWorm m_river_worm;
    void init_chunks();

    void gen_chunks_internal();
    void sync_player_pos(glm::vec3& player_pos);
    void compute_required_chunks(ChunkPosSet& required_chunks,
                                 ChunkPairVector& temp_neighbor);
    void sync_and_collect_missing_chunks(std::vector<ChunkPos>&,
                                         const ChunkPosSet&);

    void submit_new_chunks();
    void poll_finished_chunks();
    void wait_all_chunk_tasks();

public:
    World();
    ~World();

    bool can_move(const AABB& player_box) const;
    // const BlockRenderData& get_block_render_data(int x, int y ,int z);

    const std::optional<LookBlock>&
    get_look_block_pos(const std::string& name) const;
    const Chunk* get_chunk(const ChunkPos& pos) const;

    Player& get_player(const std::string& name);
    void init_world();
    int get_block(const glm::ivec3& block_pos) const;
    bool is_solid(const glm::ivec3& block_pos) const;
    bool can_pass_block(const glm::ivec3& block_pos) const;
    BlockType get_block_tpye(const glm::ivec3& block_pos) const;
    static ChunkPos chunk_pos(int world_x, int world_z);

    void need_gen();

    void set_block(const glm::ivec3& pos, unsigned id);
    void update(float delta_time);

    void push_delete_vbo(GLuint vbo);
    void push_delete_vao(GLuint vao);
    void hot_reload();

    void rebuild_world();

    float chunk_gen_fraction() const;
    int rendering_distance() const;
    void rendering_distance(int rendering_distance);
    void start_gen_thread();
    void start_server_thread();
    void stop_gen_thread();
    void stop_server_thread();
    void serever_run(std::stop_token stoken);

    CaveCarver& cave_carcer();
    RiverWorm& river_worm();
    std::vector<glm::vec4>& planes();
    std::vector<ChunkRenderSnapshot>& render_snapshots();

    glm::vec3 sunlight_dir() const;
    TickType game_tick() const;
    TickType day_tick() const;
    void day_tick(TickType tick);
    int per_tick_time() const;
    void per_tick_time(int ms);

    bool is_tick_running() const;
    void tick_running(bool run);

    void change_pool_threads(int threads);
};

} // namespace Cubed
