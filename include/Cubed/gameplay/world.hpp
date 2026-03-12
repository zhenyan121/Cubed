#pragma once
#include <unordered_map>

#include <Cubed/gameplay/chunk.hpp>



class World {
private:
    BlockRenderData m_block_render_data;
    std::unordered_map<ChunkPos , Chunk, ChunkPos::Hash> m_chunks;
    
public:
    
    World();
    ~World();

    const BlockRenderData& get_block_render_data(int x, int y ,int z);
    void init_world();
    void render();
    
    

};