#pragma once

#include <atomic>
#include <cstdint>

#include <Cubed/config.hpp>
#include <Cubed/primitive_data.hpp>
#include <Cubed/gameplay/biome.hpp>
#include <Cubed/gameplay/chunk_pos.hpp>
#include <Cubed/gameplay/block.hpp>

namespace Cubed {


class World;
// if want to use, do init_chunk(), gen_vertex_data() and 
class Chunk {
private:
    static constexpr int SIZE_X = CHUCK_SIZE;
    static constexpr int SIZE_Y = WORLD_SIZE_Y;
    static constexpr int SIZE_Z = CHUCK_SIZE;
    
    static inline const std::vector<BiomeNonAdjacent> NON_ADJACENT {{
        {Biome::PLAIN, {Biome::NONE}, Biome::PLAIN},
        {Biome::FOREST, {Biome::DESERT}, Biome::PLAIN},
        {Biome::DESERT, {Biome::MOUNTAIN, Biome::FOREST}, Biome::PLAIN},
        {Biome::MOUNTAIN, {Biome::DESERT}, Biome::PLAIN}
    }
    };
    using HeightMapArray = std::array<std::array<float, SIZE_Z>, SIZE_X>;
    std::atomic<bool> m_dirty {false};
    std::atomic<bool> m_need_upload{true};
    std::atomic<bool> m_is_on_gen_vertex_data {false};
    std::atomic<size_t> m_vertex_sum = 0;
    std::mutex m_vertexs_data_mutex;
    
    std::atomic<Biome> m_biome = Biome::PLAIN;
    ChunkPos m_chunk_pos;
    World& m_world;
    HeightMapArray m_heightmap;
    // the index is a array of block id
    std::vector<uint8_t> m_blocks;
    GLuint m_vbo = 0;
    std::vector<Vertex> m_vertexs_data;
    
    float frequency = 0.01f;
    float height = 80;

    void clear_dirty();

public:
    Chunk(World& world, ChunkPos chunk_pos);
    ~Chunk();
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&&) noexcept;
    Chunk& operator=(Chunk&&) noexcept;

    Biome get_biome() const;

    const std::vector<uint8_t>& get_chunk_blocks() const;    
    HeightMapArray get_heightmap() const;
    static int get_index(int x, int y, int z);
    static int get_index(const glm::vec3& pos);
    // Init Chunk
    // Generate Biome
    void gen_phase_one();
    // Adjust Biome
    void gen_phase_two(const std::array<const Chunk*, 4>& adj_chunks);
    // Generate Heightmap
    void gen_phase_three();
    // Adjust Height
    void gen_phase_four(const std::array<std::optional<HeightMapArray>, 4>& neighbor_heightmap);
    // Generate Block
    void gen_phase_five();
    // Adjust Block;
    void gen_phase_six(const std::array<std::optional<std::vector<uint8_t>>, 4>& neighbor_block);
    // Generate Structure
    void gen_phase_seven();
    //void gen_vertex_data();
    // 0 : (1, 0)
    // 1 : (-1, 0) 
    // 2 : (0, 1) 
    // 3 : (0, -1)
    void gen_vertex_data(const std::array<const std::vector<uint8_t>*, 4>& neighbor_block);
    void upload_to_gpu();
    
    GLuint get_vbo() const;
    size_t get_vertex_sum() const;
   
    bool is_dirty() const;
    void mark_dirty();
    
    bool is_need_upload() const;
    void need_upload();

    void set_chunk_block(int index, unsigned id);
    
};


}