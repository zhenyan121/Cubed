#pragma once
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

    const BlockRenderData& get_block_render_data(int x, int y ,int z);
    Player& get_player(const std::string& name);
    void init_world();
    void render();
    void update(float delta_time);
    

};