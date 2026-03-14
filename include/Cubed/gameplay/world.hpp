#pragma once
#include <optional>
#include <unordered_map>

#include <Cubed/gameplay/chunk.hpp>

class Player;

class World {
private:
    BlockRenderData m_block_render_data;
    std::unordered_map<ChunkPos , Chunk, ChunkPos::Hash> m_chunks;
    std::unordered_map<std::size_t, Player> m_players;
public:
    
    World();
    ~World();
    bool can_move(const glm::vec3& new_pos) const;
    const BlockRenderData& get_block_render_data(int x, int y ,int z);
    const std::optional<LookBlock>& get_look_block_pos(const std::string& name) const;
    Player& get_player(const std::string& name);
    void init_world();
    bool is_block(const glm::ivec3& block_pos) const;
    void render();
    void set_block(const glm::ivec3& pos, unsigned id);
    void update(float delta_time);
    

};