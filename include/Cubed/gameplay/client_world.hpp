#pragma once
#include "Cubed/AABB.hpp"
#include "Cubed/gameplay/block.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/client_chunk.hpp"
#include "Cubed/gameplay/client_player.hpp"

#include <deque>
#include <tbb/concurrent_unordered_map.h>
namespace Cubed {

class ClientWorld {
public:
    ClientWorld();
    ~ClientWorld();
    void init();
    void update(float delta_time);
    bool can_move(const AABB& player_box) const;
    const std::optional<LookBlock>&
    get_look_block_pos(const std::string& name) const;
    ClientPlayer& get_player();
    int get_block(const glm::ivec3& block_pos) const;
    bool is_solid(const glm::ivec3& block_pos) const;
    bool can_pass_block(const glm::ivec3& block_pos) const;
    BlockType get_block_tpye(const glm::ivec3& block_pos) const;
    void set_block(const glm::ivec3& pos, unsigned id);
    void push_delete_vbo(GLuint vbo);
    void push_delete_vao(GLuint vao);
    void hot_reload();

    void rebuild_world();

    int rendering_distance() const;
    void rendering_distance(int rendering_distance);
    void start_client_thread();
    void stop_client_thread();

    std::vector<glm::vec4>& planes();
    std::vector<ChunkRenderSnapshot>& render_snapshots();
    glm::vec3 sunlight_dir() const;

private:
    using ChunkHashMap =
        tbb::concurrent_unordered_map<ChunkPos, ClientChunk, ChunkPos::Hash>;
    ClientPlayer m_player;
    ChunkHashMap m_chunks;
    std::vector<glm::vec4> m_planes;
    std::jthread m_client_thread;

    mutable std::shared_mutex m_chunks_mutex;
    std::mutex m_delete_vbo_mutex;
    std::mutex m_delete_vao_mutex;

    std::vector<GLuint> m_pending_delete_vbo;
    std::vector<GLuint> m_pending_delete_vao;
    std::deque<ChunkPos> m_dirty_queue;
    std::vector<ChunkRenderSnapshot> m_render_snapshots;
};
} // namespace Cubed
