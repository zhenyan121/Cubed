#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <optional>
#include <unordered_map>

#include <Cubed/AABB.hpp>
#include <Cubed/gameplay/chunk.hpp>

struct ChunkRenderSnapshot {
    GLuint vbo;
    size_t vertex_count;
    glm::vec3 center;
    glm::vec3 half_extents;
};


class Player;

class World {
private:    
    glm::vec3 m_gen_player_pos{0.0f, 0.0f, 0.0f};
    std::unordered_map<ChunkPos , Chunk, ChunkPos::Hash> m_chunks;
    std::unordered_map<std::size_t, Player> m_players;
    std::vector<glm::vec4> m_planes;

    std::thread m_gen_thread;
    std::mutex m_chunks_mutex;
    std::mutex m_gen_signal_mutex; ;
    std::condition_variable m_gen_cv;
    std::atomic<bool> m_gen_running{false};
    std::atomic<bool> m_need_gen_chunk{false};

    std::vector<ChunkPos> m_dirty_queue;
    std::vector<ChunkRenderSnapshot> m_render_snapshots;

    void gen_chunks_internal();

    void start_gen_thread();
    void stop_gen_thread();

public:
    
    World();
    ~World();

    bool can_move(const AABB& player_box) const;
    //const BlockRenderData& get_block_render_data(int x, int y ,int z);
    const std::optional<LookBlock>& get_look_block_pos(const std::string& name) const;
    Player& get_player(const std::string& name);
    void init_world();
    bool is_aabb_in_frustum(const glm::vec3& center, const glm::vec3& half_extents);

    int get_block(const glm::ivec3& block_pos) const;
    bool is_block(const glm::ivec3& block_pos) const;
    
    ChunkPos chunk_pos(int world_x, int world_z) const;

    void need_gen();
    void render(const glm::mat4& mvp_matrix);
    
    void set_block(const glm::ivec3& pos, unsigned id);
    void update(float delta_time);
    

};