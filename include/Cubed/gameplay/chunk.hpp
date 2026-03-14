#pragma once

#include <cstdint>

#include <Cubed/config.hpp>
#include <Cubed/gameplay/chunk_status.hpp>
#include <Cubed/gameplay/block.hpp>

class World;

class Chunk {
private:
    ChunkPos m_chunk_pos;
    World& m_world;
    // the index is a array of block id
    std::vector<uint8_t> m_blocks;
    GLuint m_vbo = 0;
    std::vector<Vertex> m_vertexs_data;

public:
    Chunk(World& world, ChunkPos chunk_pos);
    ~Chunk();
    const std::vector<uint8_t>& get_chunk_blocks() const;    
    
    static int get_index(int x, int y, int z);
    
    void gen_vertex_data();
    GLuint get_vbo() const;
    const std::vector<Vertex>& get_vertex_data() const;
    void init_chunk();

    void set_chunk_block(int index, unsigned id);
    
};