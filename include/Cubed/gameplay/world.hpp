#pragma once
#include <optional>
#include <unordered_map>

#include <Cubed/AABB.hpp>
#include <Cubed/gameplay/chunk.hpp>

class Player;

class World {
private:
    bool need_gen_chunk = false;    

    BlockRenderData m_block_render_data;
    std::unordered_map<ChunkPos , Chunk, ChunkPos::Hash> m_chunks;
    std::unordered_map<std::size_t, Player> m_players;
    std::vector<glm::vec4> m_planes;

    std::pair<int, int> chunk_pos(int world_x, int world_z);
    void gen_chunks();

    

public:
    
    World();
    ~World();
    bool can_move(const AABB& player_box) const;
    const BlockRenderData& get_block_render_data(int x, int y ,int z);
    const std::optional<LookBlock>& get_look_block_pos(const std::string& name) const;
    Player& get_player(const std::string& name);
    void init_world();
    bool is_aabb_in_frustum(const glm::vec3& center, const glm::vec3& half_extents);
    bool is_block(const glm::ivec3& block_pos) const;
    
    void need_gen();
    void render(const glm::mat4& mvp_matrix);
    
    void set_block(const glm::ivec3& pos, unsigned id);
    void update(float delta_time);
    

};