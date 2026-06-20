#include "Cubed/gameplay/chunk.hpp"

#include "Cubed/gameplay/world.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"

#include <utility>

namespace Cubed {

Chunk::Chunk(World& world, ChunkPos chunk_pos)
    : m_chunk_pos(chunk_pos), m_world(world) {
    for (int i = 0; i < VERTEX_DATA_SUM; i++) {
        m_vertex_data.emplace_back(m_world);
    }
}

Chunk::~Chunk() {}

Chunk::Chunk(Chunk&& other) noexcept
    : m_dirty(other.is_dirty()), m_need_upload(other.m_need_upload.load()),
      m_is_on_gen_vertex_data(other.m_is_on_gen_vertex_data.load()),
      m_biome(other.m_biome.load()), m_chunk_pos(std::move(other.m_chunk_pos)),
      m_world(other.m_world), m_heightmap(std::move(other.m_heightmap)),
      m_blocks(std::move(other.m_blocks)),
      m_vertex_data(std::move(other.m_vertex_data)), m_seed(other.m_seed),
      m_conditions(other.m_conditions) {}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    // Logger::info("other Chunk pos {} {} in Chunk& Chunk::operator=(Chunk&&
    // other) this {}", other.m_chunk_pos.x, other.m_chunk_pos.z,
    // static_cast<const void*>(&other));

    m_chunk_pos = std::move(other.m_chunk_pos);
    m_heightmap = std::move(other.m_heightmap);
    m_blocks = std::move(other.m_blocks);
    m_dirty = other.is_dirty();
    m_vertex_data = std::move(other.m_vertex_data);
    m_biome = other.m_biome.load();
    m_is_on_gen_vertex_data = other.m_is_on_gen_vertex_data.load();
    m_need_upload = other.m_need_upload.load();
    m_seed = other.m_seed;
    m_conditions = other.m_conditions;
    return *this;
}

std::tuple<int, int, int> Chunk::world_to_block(int world_x, int world_y,
                                                int world_z, int chunk_x,
                                                int chunk_z) {
    int x, y, z;
    y = world_y;
    x = world_x - chunk_x * CHUNK_SIZE;
    z = world_z - chunk_z * CHUNK_SIZE;
    return {x, y, z};
}

std::tuple<int, int, int> Chunk::world_to_block(const glm::ivec3& block_pos,
                                                ChunkPos chunk_pos) {
    return world_to_block(block_pos.x, block_pos.y, block_pos.z, chunk_pos.x,
                          chunk_pos.z);
}

std::tuple<int, int, int> Chunk::block_to_world(int x, int y, int z,
                                                int chunk_x, int chunk_z) {
    int world_x = x + chunk_x * CHUNK_SIZE;
    int world_z = z + chunk_z * CHUNK_SIZE;
    int world_y = y;
    return {world_x, world_y, world_z};
}
std::tuple<int, int, int> Chunk::block_to_world(const glm::ivec3& block_pos,
                                                ChunkPos chunk_pos) {
    return block_to_world(block_pos.x, block_pos.y, block_pos.z, chunk_pos.x,
                          chunk_pos.z);
}

BiomeType Chunk::get_biome() const { return m_biome.load(); }

ChunkPos Chunk::get_chunk_pos() const { return m_chunk_pos; }

const std::vector<BlockType>& Chunk::get_chunk_blocks() const {
    return m_blocks;
}

HeightMapArray Chunk::get_heightmap() const {
    // Logger::info("Chunk pos {} {} in get_heightmap this {}", m_chunk_pos.x,
    // m_chunk_pos.z, static_cast<const void*>(this));
    return m_heightmap;
}

int Chunk::index(int x, int y, int z) {
    ASSERT(!(x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
             z >= CHUNK_SIZE));
    if ((x * WORLD_SIZE_Y + y) * CHUNK_SIZE + z < 0 ||
        (x * WORLD_SIZE_Y + y) * CHUNK_SIZE + z >=
            CHUNK_SIZE * CHUNK_SIZE * WORLD_SIZE_Y) {
        Logger::error("block pos x {} y {} z {} range error", x, y, z);
        ASSERT(0);
    }
    return (x * WORLD_SIZE_Y + y) * CHUNK_SIZE + z;
}

int Chunk::index(const glm::vec3& pos) {
    return Chunk::index(pos.x, pos.y, pos.z);
}

void Chunk::gen_vertex_data(const OptionalBlockVectorArray& neighbor_block) {
    if (m_is_on_gen_vertex_data) {
        return;
    }
    m_is_on_gen_vertex_data = true;
    std::lock_guard lk(m_vertexs_data_mutex);

    for (auto& data : m_vertex_data) {
        data.m_vertices.clear();
    }

    gen_vertices(neighbor_block);
    for (auto& data : m_vertex_data) {
        data.update_sum();
    }
    m_need_upload = true;
    m_is_on_gen_vertex_data = false;
}

GLuint Chunk::get_normal_vao() const { return m_vertex_data[0].m_vao; }

size_t Chunk::get_normal_vertices_sum() const {
    if (m_vertex_data[0].m_sum == 0) {
        Logger::warn("m_normal_vertices_sum is 0");
    }
    return m_vertex_data[0].m_sum.load();
}

GLuint Chunk::get_cross_vao() const { return m_vertex_data[1].m_vao; }
size_t Chunk::get_cross_vertices_sum() const {
    return m_vertex_data[1].m_sum.load();
}

GLuint Chunk::get_normal_discard_vao() const { return m_vertex_data[2].m_vao; }
size_t Chunk::get_normal_discard_vertices_sum() const {
    return m_vertex_data[2].m_sum.load();
}

GLuint Chunk::get_normal_blend_vao() const { return m_vertex_data[3].m_vao; }
size_t Chunk::get_normal_blend_vertices_sum() const {
    return m_vertex_data[3].m_sum.load();
}

GLuint Chunk::get_water_vao() const { return m_vertex_data[4].m_vao; }
size_t Chunk::get_water_vertices_sum() const {
    return m_vertex_data[4].m_sum.load();
}

void Chunk::gen_phase_one() {
    m_generator = std::make_unique<ChunkGenerator>(*this);
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->assign_chunk_biome();
    m_seed = m_generator->chunk_seed();
}

void Chunk::gen_phase_two(const std::array<const Chunk*, 8>& adj_chunks) {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    // m_generator->resolve_biome_adjacency_conflict(adj_chunks);
}

void Chunk::gen_phase_three() {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->generate_heightmap();
}

void Chunk::gen_phase_four(
    const std::array<std::optional<HeightMapArray>, 8>& neighbor_heightmap,
    const std::array<BiomeType, 8>& neighbor_biome) {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    // m_generator->blend_heightmap_boundaries(neighbor_heightmap,
    // neighbor_biome);
}

void Chunk::gen_phase_five() {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->generate_terrain_blocks();
}

void Chunk::gen_phase_six(
    const std::array<std::optional<std::vector<BlockType>>, 4>&
        neighbor_block) {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    // This must be fully completed before any other operations can proceed!
    m_generator->blend_surface_blocks_borders(neighbor_block);
}

void Chunk::gen_phase_seven() {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->ocean_build();
    m_generator->generate_river();
    m_generator->generate_cave();

    m_generator->generate_vegetation();
    mark_dirty();
    m_generator = nullptr;
}

void Chunk::upload_to_gpu() {

    ASSERT(is_need_upload());

    std::lock_guard lk(m_vertexs_data_mutex);

    for (auto& data : m_vertex_data) {
        data.upload();
    }

    // after fininshed it, can use
    clear_dirty();
    m_need_upload = false;
}

bool Chunk::is_dirty() const { return m_dirty.load(); }

void Chunk::mark_dirty() { m_dirty = true; }

void Chunk::clear_dirty() { m_dirty = false; }

bool Chunk::is_need_upload() const { return m_need_upload.load(); }

void Chunk::need_upload() { m_need_upload = true; }

void Chunk::set_chunk_block(int index, unsigned id) {
    m_blocks[index] = id;
    mark_dirty();
}

ChunkPos Chunk::chunk_pos() const { return m_chunk_pos; }

BiomeType Chunk::biome() const { return m_biome; }

void Chunk::biome(BiomeType b) { m_biome = b; }

HeightMapArray& Chunk::heightmap() { return m_heightmap; }
std::vector<BlockType>& Chunk::blocks() { return m_blocks; }
World& Chunk::world() { return m_world; }
unsigned Chunk::seed() const {
    if (m_seed == 0) {
        Logger::warn("Seed Not Generator");
    }
    return m_seed;
}

BiomeConditions& Chunk::conditions() { return m_conditions; }

void Chunk::gen_vertices(const OptionalBlockVectorArray& neighbor_block) {
    static const glm::ivec3 DIR[6] = {{0, 0, 1},  {1, 0, 0}, {0, 0, -1},
                                      {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};

    for (int x = 0; x < SIZE_X; x++) {
        for (int y = 0; y < SIZE_Y; y++) {
            for (int z = 0; z < SIZE_Z; z++) {
                int world_x = x + m_chunk_pos.x * CHUNK_SIZE;
                int world_z = z + m_chunk_pos.z * CHUNK_SIZE;
                int world_y = y;
                int cur_id = m_blocks[index(x, y, z)];
                // air
                if (cur_id == 0) {
                    continue;
                }

                for (int face = 0; face < 6; face++) {
                    int nx = x + DIR[face].x;
                    int ny = y + DIR[face].y;
                    int nz = z + DIR[face].z;
                    bool neighbor_culled = false;

                    if (nx < 0 || nx >= SIZE_X || ny < 0 || ny >= SIZE_Y ||
                        nz < 0 || nz >= SIZE_Z) {

                        int world_nx = world_x + DIR[face].x;
                        int world_ny = world_y + DIR[face].y;
                        int world_nz = world_z + DIR[face].z;

                        auto [neighbor_x, neighbor_z] =
                            World::chunk_pos(world_nx, world_nz);

                        auto is_culled =
                            [&](const std::optional<std::vector<BlockType>>&
                                    chunk_blocks) {
                                if (chunk_blocks == std::nullopt) {
                                    return true;
                                }
                                int x, y, z;
                                y = world_ny;
                                x = world_nx - neighbor_x * CHUNK_SIZE;
                                z = world_nz - neighbor_z * CHUNK_SIZE;
                                if (x < 0 || y < 0 || z < 0 ||
                                    x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
                                    z >= CHUNK_SIZE) {
                                    return false;
                                }

                                int idx = Chunk::index(x, y, z);
                                // not init
                                if (static_cast<size_t>(idx) >=
                                    chunk_blocks->size()) {
                                    // Logger::warn("not init");
                                    return true;
                                }
                                auto id = (*chunk_blocks)[idx];
                                // transparent
                                if (BlockManager::is_transparent(id)) {
                                    if (id == cur_id) {
                                        return true;
                                    } else {
                                        return false;
                                    }

                                } else {
                                    return true;
                                }
                            };

                        if (m_chunk_pos.x + 1 == neighbor_x) {
                            neighbor_culled = is_culled(neighbor_block[0]);
                        } else if (m_chunk_pos.x - 1 == neighbor_x) {
                            neighbor_culled = is_culled(neighbor_block[1]);
                        } else if (m_chunk_pos.z + 1 == neighbor_z) {
                            neighbor_culled = is_culled(neighbor_block[2]);
                        } else if (m_chunk_pos.z - 1 == neighbor_z) {
                            neighbor_culled = is_culled(neighbor_block[3]);
                        }
                        // neighbor_cull = m_world.is_block(glm::ivec3(world_x,
                        // world_y, world_z) + DIR[face]);
                    } else {
                        auto neighbor_id = m_blocks[index(nx, ny, nz)];
                        // transparent block
                        if (!BlockManager::is_transparent(neighbor_id)) {
                            neighbor_culled = true;
                        } else {
                            if (neighbor_id == cur_id) {
                                neighbor_culled = true;
                            } else {
                                neighbor_culled = false;
                            }
                        }
                    }

                    if (neighbor_culled) {
                        continue;
                    }
                    if (BlockManager::is_cross_plane(cur_id)) {
                        gen_cross_plane_vertices(world_x, world_y, world_z,
                                                 cur_id);
                    }
                    for (int i = 0; i < 6; i++) {
                        Vertex3D vex = {
                            VERTICES_POS[face][i][0] + (float)world_x * 1.0f,
                            VERTICES_POS[face][i][1] + (float)world_y * 1.0f,
                            VERTICES_POS[face][i][2] + (float)world_z * 1.0f,
                            TEX_COORDS[face][i][0],
                            TEX_COORDS[face][i][1],

                            static_cast<float>(cur_id * 6 + face),

                            NORMALS[face][i][0],
                            NORMALS[face][i][1],
                            NORMALS[face][i][2],
                            BlockManager::roughness(cur_id)

                        };
                        if (BlockManager::is_transparent(cur_id)) {
                            if (BlockManager::is_discard(cur_id) &&
                                BlockManager::is_blend(cur_id)) {
                                Logger::warn(
                                    "Block id {} is both discard and blend is "
                                    "must only one can true !!!",
                                    cur_id);
                            }
                            if (BlockManager::is_discard(cur_id)) {
                                m_vertex_data[2].m_vertices.emplace_back(vex);
                            } else if (BlockManager::is_blend(cur_id)) {
                                if (cur_id == 7) {
                                    m_vertex_data[4].m_vertices.emplace_back(
                                        vex);
                                } else {
                                    m_vertex_data[3].m_vertices.emplace_back(
                                        vex);
                                }

                            } else {
                                Logger::warn("Id {} is transparent but not "
                                             "discard or blend",
                                             cur_id);
                                m_vertex_data[3].m_vertices.emplace_back(vex);
                            }

                        } else {
                            m_vertex_data[0].m_vertices.emplace_back(vex);
                        }
                    }
                }
            }
        }
    }
}
void Chunk::gen_cross_plane_vertices(int world_x, int world_y, int world_z,
                                     BlockType id) {

    if (!BlockManager::is_cross_plane(id)) {
        Logger::warn("Block {} {} {} id {} is not cross plane", world_x,
                     world_y, world_z, id);
        return;
    }
    for (int face = 0; face < 2; face++) {
        for (int i = 0; i < 6; i++) {
            Vertex3D vex = {
                CROSS_VERTICES_POS[face][i][0] + (float)world_x * 1.0f,
                CROSS_VERTICES_POS[face][i][1] + (float)world_y * 1.0f,
                CROSS_VERTICES_POS[face][i][2] + (float)world_z * 1.0f,
                CROSS_TEX_COORDS[face][i][0],
                CROSS_TEX_COORDS[face][i][1],
                static_cast<float>(BlockManager::cross_plane_index(id)),
                CROSS_NORMALS[face][i][0],
                CROSS_NORMALS[face][i][1],
                CROSS_NORMALS[face][i][2],
                BlockManager::roughness(id)

            };
            m_vertex_data[1].m_vertices.emplace_back(vex);
        }
    }
}

// Logger::info("Cross Sum {}", m_cross_vertices_sum.load());

} // namespace Cubed
