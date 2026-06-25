#pragma once
#include "Cubed/constants.hpp"
#include "Cubed/gameplay/biome.hpp"
#include "Cubed/gameplay/block.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/vertex_data.hpp"
#include "world/chunk_data.pb.h"

#include <atomic>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <mutex>
namespace Cubed {
class ClientWorld;
struct ChunkRenderSnapshot {
    GLuint normal_vao;
    size_t normal_vertices_count;
    GLuint cross_vao;
    size_t cross_vertices_count;
    GLuint normal_discard_vao;
    size_t normal_discard_vertices_count;
    GLuint normal_blend_vao;
    size_t normal_blend_vertices_count;
    GLuint water_vao;
    size_t water_vertices_count;
    glm::vec3 center;
    glm::vec3 half_extents;
};
class ClientChunk {
public:
    ClientChunk(ClientWorld& world);
    ~ClientChunk();
    ClientChunk(const ClientChunk&) = delete;
    ClientChunk& operator=(const ClientChunk&) = delete;
    ClientChunk(ClientChunk&&) noexcept;
    ClientChunk& operator=(ClientChunk&&) noexcept;

    static int index(int x, int y, int z);
    static int index(const glm::vec3& pos);
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
    void receive_chunk(const ChunkDataRsp& data);
    void gen_vertex_data(const OptionalBlockVectorArray& neighbor_block);
    void upload_to_gpu();

    GLuint get_normal_vao() const;
    size_t get_normal_vertices_sum() const;

    GLuint get_cross_vao() const;
    size_t get_cross_vertices_sum() const;

    GLuint get_normal_discard_vao() const;
    size_t get_normal_discard_vertices_sum() const;

    GLuint get_normal_blend_vao() const;
    size_t get_normal_blend_vertices_sum() const;

    GLuint get_water_vao() const;
    size_t get_water_vertices_sum() const;

    bool is_dirty() const;
    void mark_dirty();

    bool is_need_upload() const;
    void need_upload();

    void set_chunk_block(int index, unsigned id);
    bool is_temp_chunk() const;
    ChunkPos chunk_pos() const;
    BiomeType biome() const;
    void biome(BiomeType b);
    std::vector<BlockType>& blocks();
    ClientWorld& world();
    unsigned seed() const;

private:
    struct FaceKey {
        BlockType block_id = 0;
        int face = -1; // 0-5, used to index NORMALS/TANGENTS/TEX_COORDS

        bool valid() const { return block_id != 0; }
        bool operator==(const FaceKey& o) const {
            return block_id == o.block_id && face == o.face;
        }
        bool operator!=(const FaceKey& o) const { return !(*this == o); }
    };

    static constexpr int SIZE_X = CHUNK_SIZE;
    static constexpr int SIZE_Y = WORLD_SIZE_Y;
    static constexpr int SIZE_Z = CHUNK_SIZE;
    static constexpr int BLOCK_SIZE = SIZE_X * SIZE_Y * SIZE_Z;
    static constexpr int VERTEX_DATA_SUM = 5;
    std::atomic<bool> m_dirty{false};
    std::atomic<bool> m_need_upload{true};
    std::atomic<bool> m_is_on_gen_vertex_data{false};
    std::atomic<BiomeType> m_biome = BiomeType::PLAIN;
    std::mutex m_vertexs_data_mutex;
    ChunkPos m_chunk_pos;
    ClientWorld& m_world;
    // the index is a array of block id
    std::vector<BlockType> m_blocks;
    /*
    0 - normal
    1 - cross_plane
    2 - normal_discard
    3 - transparent and blend
    4 - water
    */
    std::vector<VertexData> m_vertex_data;
    unsigned m_seed = 0;
    void clear_dirty();
    void gen_vertices(const OptionalBlockVectorArray& neighbor_block);
    void gen_cross_plane_vertices(int world_x, int world_y, int world_z,
                                  BlockType id);
    void emit_quad(int axis, int face_dir, int layer, int i, int j, int w,
                   int h, int u_axis, int v_axis, FaceKey key);
};
} // namespace Cubed