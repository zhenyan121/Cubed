#pragma once

#include <atomic>
#include <cstdint>

#include <Cubed/config.hpp>
#include <Cubed/gameplay/biome.hpp>
#include <Cubed/gameplay/chunk_pos.hpp>
#include <Cubed/gameplay/block.hpp>

class World;
// if want to use, do init_chunk(), gen_vertex_data() and 
class Chunk {
private:
    std::atomic<bool> m_dirty {false};
    
    static constexpr int SIZE_X = CHUCK_SIZE;
    static constexpr int SIZE_Y = WORLD_SIZE_Y;
    static constexpr int SIZE_Z = CHUCK_SIZE;
    
    Biome m_biome = Biome::PLAIN;
    ChunkPos m_chunk_pos;
    World& m_world;
    // the index is a array of block id
    std::vector<uint8_t> m_blocks;
    GLuint m_vbo = 0;
    std::vector<Vertex> m_vertexs_data;
    
    float frequency = 0.01f;
    float height = 80;

    void clear_dirty();

    void resolve_biome();
    void resolve_blocks();

public:
    Chunk(World& world, ChunkPos chunk_pos);
    ~Chunk();
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&&);
    Chunk& operator=(Chunk&&);

    Biome get_biome() const;

    const std::vector<uint8_t>& get_chunk_blocks() const;    
    
    static int get_index(int x, int y, int z);
    static int get_index(const glm::vec3& pos);
    void init_chunk();
    //void gen_vertex_data();
    // 0 : (1, 0)
    // 1 : (-1, 0) 
    // 2 : (0, 1) 
    // 3 : (0, -1)
    void gen_vertex_data(const std::array<const std::vector<uint8_t>*, 4>& neighbor_block);
    void upload_to_gpu();
    
    GLuint get_vbo() const;
    const std::vector<Vertex>& get_vertex_data() const;
   
    bool is_dirty() const;
    void mark_dirty();
    
    void set_chunk_block(int index, unsigned id);
    
};