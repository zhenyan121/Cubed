#pragma once

#include "Cubed/gameplay/biome.hpp"
#include "Cubed/gameplay/block.hpp"
#include "Cubed/gameplay/chunk_generator.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/primitive_data.hpp"

#include <atomic>
#include <cstdint>

namespace Cubed {

class World;
// if want to use, do init_chunk(), gen_vertex_data() and
class Chunk {
private:
    static constexpr int SIZE_X = CHUCK_SIZE;
    static constexpr int SIZE_Y = WORLD_SIZE_Y;
    static constexpr int SIZE_Z = CHUCK_SIZE;

    std::atomic<bool> m_dirty{false};
    std::atomic<bool> m_need_upload{true};
    std::atomic<bool> m_is_on_gen_vertex_data{false};
    std::atomic<size_t> m_vertex_sum = 0;
    std::atomic<BiomeType> m_biome = BiomeType::PLAIN;
    std::mutex m_vertexs_data_mutex;

    std::unique_ptr<ChunkGenerator> m_generator;

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

    BiomeType get_biome() const;
    ChunkPos get_chunk_pos() const;
    const std::vector<uint8_t>& get_chunk_blocks() const;
    HeightMapArray get_heightmap() const;
    static int get_index(int x, int y, int z);
    static int get_index(const glm::vec3& pos);
    // Init Chunk
    // Determine biome from temperature and humidity noise
    void gen_phase_one();
    // Resolve biome adjacency conflicts with neighbor chunks
    void gen_phase_two(const std::array<const Chunk*, 4>& adj_chunks);
    // Generate heightmap using biome-specific noise
    void gen_phase_three();
    // Blend heightmap with neighbors for smooth transitions
    void gen_phase_four(
        const std::array<std::optional<HeightMapArray>, 4>& neighbor_heightmap);
    // Generate terrain blocks from heightmap and biome
    void gen_phase_five();
    // Blend surface blocks at chunk borders with neighbors
    void gen_phase_six(const std::array<std::optional<std::vector<uint8_t>>, 4>&
                           neighbor_block);
    // Generate biome-specific vegetation/structures
    void gen_phase_seven();
    // void gen_vertex_data();
    //  0 : (1, 0)
    //  1 : (-1, 0)
    //  2 : (0, 1)
    //  3 : (0, -1)
    void gen_vertex_data(
        const std::array<const std::vector<uint8_t>*, 4>& neighbor_block);
    void upload_to_gpu();

    GLuint get_vbo() const;
    size_t get_vertex_sum() const;

    bool is_dirty() const;
    void mark_dirty();

    bool is_need_upload() const;
    void need_upload();

    void set_chunk_block(int index, unsigned id);

    ChunkPos chunk_pos() const;
    BiomeType biome() const;
    void biome(BiomeType b);
    HeightMapArray& heightmap();
    std::vector<uint8_t>& blocks();
};

} // namespace Cubed