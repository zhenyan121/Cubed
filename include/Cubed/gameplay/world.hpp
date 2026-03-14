#pragma once
#include <unordered_map>

#include <Cubed/gameplay/chunk.hpp>

class Player;

class World {
private:
    BlockRenderData m_block_render_data;
    glm::ivec3 last_block_pos = glm::ivec3(0, 0, 0);
    std::unordered_map<ChunkPos , Chunk, ChunkPos::Hash> m_chunks;
    std::unordered_map<std::size_t, Player> m_players;
public:
    
    World();
    ~World();

    const BlockRenderData& get_block_render_data(int x, int y ,int z);
    const glm::ivec3& get_last_block_pos() const;
    Player& get_player(const std::string& name);
    void init_world();
    bool is_block(const glm::ivec3& block_pos) const;
    void mark_looked_block(const glm::ivec3& block_pos);
    void render();
    void update(float delta_time);
    

};