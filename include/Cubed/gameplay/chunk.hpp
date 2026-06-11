#pragma once

#include "Cubed/gameplay/biome.hpp"
#include "Cubed/gameplay/block.hpp"
#include "Cubed/gameplay/chunk_generator.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/vertex_data.hpp"

#include <atomic>
#include <mutex>
namespace Cubed {

class World;
// if want to use, do init_chunk(), gen_vertex_data() and
class Chunk {
private:
    static constexpr int SIZE_X = CHUNK_SIZE;
    static constexpr int SIZE_Y = WORLD_SIZE_Y;
    static constexpr int SIZE_Z = CHUNK_SIZE;
    static constexpr int VERTEX_DATA_SUM = 4;
    std::atomic<bool> m_dirty{false};
    std::atomic<bool> m_need_upload{true};
    std::atomic<bool> m_is_on_gen_vertex_data{false};
    std::atomic<BiomeType> m_biome = BiomeType::PLAIN;
    std::mutex m_vertexs_data_mutex;

    std::unique_ptr<ChunkGenerator> m_generator;

    ChunkPos m_chunk_pos;
    World& m_world;
    HeightMapArray m_heightmap;
    // the index is a array of block id
    std::vector<BlockType> m_blocks;

    /*
    0 - normal
    1 - cross_plane
    2 - normal_discard
    3 - transparent and blend
    */
    std::vector<VertexData> m_vertex_data;
    float frequency = 0.01f;
    float height = 80;
    unsigned m_seed = 0;

    BiomeConditions m_conditions;

    void clear_dirty();
    void gen_vertices(
        const std::array<const std::vector<BlockType>*, 4>& neighbor_block);
    void gen_cross_plane_vertices(int world_x, int world_y, int world_z,
                                  BlockType id);

public:
    Chunk(World& world, ChunkPos chunk_pos);
    ~Chunk();
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&&) noexcept;
    Chunk& operator=(Chunk&&) noexcept;

    static std::tuple<int, int, int> world_to_block(int world_x, int world_y,
                                                    int world_z, int chunk_x,
                                                    int chunk_z);
    static std::tuple<int, int, int> world_to_block(const glm::ivec3& block_pos,
                                                    ChunkPos chunk_pos);
    static std::tuple<int, int, int> block_to_world(int x, int y, int z,
                                                    int chunk_x, int chunk_z);
    static std::tuple<int, int, int> block_to_world(const glm::ivec3& block_pos,
                                                    ChunkPos chunk_pos);
    BiomeType get_biome() const;
    ChunkPos get_chunk_pos() const;
    const std::vector<BlockType>& get_chunk_blocks() const;
    HeightMapArray get_heightmap() const;
    static int index(int x, int y, int z);
    static int index(const glm::vec3& pos);
    // Init Chunk
    // Determine biome from temperature and humidity noise
    void gen_phase_one();
    // Resolve biome adjacency conflicts with neighbor chunks
    void gen_phase_two(const std::array<const Chunk*, 8>& adj_chunks);
    // Generate heightmap using biome-specific noise
    void gen_phase_three();
    // Blend heightmap with neighbors for smooth transitions
    void gen_phase_four(
        const std::array<std::optional<HeightMapArray>, 8>& neighbor_heightmap,
        const std::array<BiomeType, 8>& neighbor_biome);
    // Generate terrain blocks from heightmap and biome
    void gen_phase_five();
    // Blend surface blocks at chunk borders with neighbors
    void gen_phase_six(const std::array<std::optional<std::vector<BlockType>>,
                                        4>& neighbor_block);
    // Generate biome-specific vegetation/structures
    void gen_phase_seven();
    // void gen_vertex_data();
    //  0 : (1, 0)
    //  1 : (-1, 0)
    //  2 : (0, 1)
    //  3 : (0, -1)
    void gen_vertex_data(
        const std::array<const std::vector<BlockType>*, 4>& neighbor_block);
    void upload_to_gpu();

    GLuint get_normal_vbo() const;
    size_t get_normal_vertices_sum() const;

    GLuint get_cross_vbo() const;
    size_t get_cross_vertices_sum() const;

    GLuint get_normal_discard_vbo() const;
    size_t get_normal_discard_vertices_sum() const;

    GLuint get_normal_blend_vbo() const;
    size_t get_normal_blend_vertices_sum() const;

    bool is_dirty() const;
    void mark_dirty();

    bool is_need_upload() const;
    void need_upload();

    void set_chunk_block(int index, unsigned id);

    ChunkPos chunk_pos() const;
    BiomeType biome() const;
    void biome(BiomeType b);
    HeightMapArray& heightmap();
    std::vector<BlockType>& blocks();
    World& world();
    unsigned seed() const;
    BiomeConditions& conditions();
};

} // namespace Cubed