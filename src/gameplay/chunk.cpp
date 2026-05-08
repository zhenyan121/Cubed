#include "Cubed/gameplay/chunk.hpp"

#include "Cubed/gameplay/world.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"

#include <utility>

namespace Cubed {

Chunk::Chunk(World& world, ChunkPos chunk_pos)
    : m_chunk_pos(chunk_pos), m_world(world) {}

Chunk::~Chunk() {
    if (m_vbo != 0) {
        m_world.push_delete_vbo(m_vbo);
    }
}

Chunk::Chunk(Chunk&& other) noexcept
    : m_dirty(other.is_dirty()), m_need_upload(other.m_need_upload.load()),
      m_is_on_gen_vertex_data(other.m_is_on_gen_vertex_data.load()),
      m_vertex_sum(other.m_vertex_sum.load()), m_biome(other.m_biome.load()),
      m_chunk_pos(std::move(other.m_chunk_pos)), m_world(other.m_world),
      m_heightmap(std::move(other.m_heightmap)),
      m_blocks(std::move(other.m_blocks)), m_vbo(other.m_vbo),
      m_vertexs_data(std::move(other.m_vertexs_data)) {
    other.m_vbo = 0;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    // Logger::info("other Chunk pos {} {} in Chunk& Chunk::operator=(Chunk&&
    // other) this {}", other.m_chunk_pos.x, other.m_chunk_pos.z,
    // static_cast<const void*>(&other));
    m_vbo = other.m_vbo;
    other.m_vbo = 0;
    m_chunk_pos = std::move(other.m_chunk_pos);
    m_heightmap = std::move(other.m_heightmap);
    m_blocks = std::move(other.m_blocks);
    m_dirty = other.is_dirty();
    m_vertexs_data = std::move(other.m_vertexs_data);
    m_biome = other.m_biome.load();
    m_is_on_gen_vertex_data = other.m_is_on_gen_vertex_data.load();
    m_need_upload = other.m_need_upload.load();
    m_vertex_sum = other.m_vertex_sum.load();
    return *this;
}

BiomeType Chunk::get_biome() const { return m_biome.load(); }

ChunkPos Chunk::get_chunk_pos() const { return m_chunk_pos; }

const std::vector<uint8_t>& Chunk::get_chunk_blocks() const { return m_blocks; }

HeightMapArray Chunk::get_heightmap() const {
    // Logger::info("Chunk pos {} {} in get_heightmap this {}", m_chunk_pos.x,
    // m_chunk_pos.z, static_cast<const void*>(this));
    return m_heightmap;
}

int Chunk::get_index(int x, int y, int z) {
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

int Chunk::get_index(const glm::vec3& pos) {
    return Chunk::get_index(pos.x, pos.y, pos.z);
}

void Chunk::gen_vertex_data(
    const std::array<const std::vector<uint8_t>*, 4>& neighbor_block) {
    if (m_is_on_gen_vertex_data) {
        return;
    }
    m_is_on_gen_vertex_data = true;
    std::lock_guard lk(m_vertexs_data_mutex);
    m_vertexs_data.clear();

    static const glm::ivec3 DIR[6] = {{0, 0, 1},  {1, 0, 0}, {0, 0, -1},
                                      {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};

    for (int x = 0; x < SIZE_X; x++) {
        for (int y = 0; y < SIZE_Y; y++) {
            for (int z = 0; z < SIZE_Z; z++) {
                int world_x = x + m_chunk_pos.x * CHUNK_SIZE;
                int world_z = z + m_chunk_pos.z * CHUNK_SIZE;
                int world_y = y;
                int cur_id = m_blocks[get_index(x, y, z)];
                // air
                if (cur_id == 0) {
                    continue;
                }

                for (int face = 0; face < 6; face++) {
                    int nx = x + DIR[face].x;
                    int ny = y + DIR[face].y;
                    int nz = z + DIR[face].z;
                    bool neighbor_cull = false;

                    if (nx < 0 || nx >= SIZE_X || ny < 0 || ny >= SIZE_Y ||
                        nz < 0 || nz >= SIZE_Z) {

                        int world_nx = world_x + DIR[face].x;
                        int world_ny = world_y + DIR[face].y;
                        int world_nz = world_z + DIR[face].z;

                        auto [neighbor_x, neighbor_z] =
                            World::chunk_pos(world_nx, world_nz);

                        auto is_cull =
                            [&](const std::vector<uint8_t>* chunk_blocks) {
                                if (chunk_blocks == nullptr) {
                                    return false;
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

                                int idx = Chunk::get_index(x, y, z);
                                // not init
                                if (static_cast<size_t>(idx) >=
                                    chunk_blocks->size()) {
                                    Logger::warn("not init");
                                    return false;
                                }
                                auto id = (*chunk_blocks)[idx];
                                if (is_in_transparent_map(id)) {
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
                            neighbor_cull = is_cull(neighbor_block[0]);
                        } else if (m_chunk_pos.x - 1 == neighbor_x) {
                            neighbor_cull = is_cull(neighbor_block[1]);
                        } else if (m_chunk_pos.z + 1 == neighbor_z) {
                            neighbor_cull = is_cull(neighbor_block[2]);
                        } else if (m_chunk_pos.z - 1 == neighbor_z) {
                            neighbor_cull = is_cull(neighbor_block[3]);
                        }
                        // neighbor_cull = m_world.is_block(glm::ivec3(world_x,
                        // world_y, world_z) + DIR[face]);
                    } else {
                        auto id = m_blocks[get_index(nx, ny, nz)];
                        if (!is_in_transparent_map(id)) {
                            neighbor_cull = true;
                        } else {
                            if (id == cur_id) {
                                neighbor_cull = true;
                            } else {
                                neighbor_cull = false;
                            }
                        }
                    }

                    if (neighbor_cull) {
                        continue;
                    }
                    for (int i = 0; i < 6; i++) {
                        Vertex vex = {
                            VERTICES_POS[face][i][0] + (float)world_x * 1.0f,
                            VERTICES_POS[face][i][1] + (float)world_y * 1.0f,
                            VERTICES_POS[face][i][2] + (float)world_z * 1.0f,
                            TEX_COORDS[face][i][0],
                            TEX_COORDS[face][i][1],
                            static_cast<float>(cur_id * 6 + face)

                        };
                        m_vertexs_data.emplace_back(vex);
                    }
                }
            }
        }
    }
    m_vertex_sum = m_vertexs_data.size();
    m_need_upload = true;
    m_is_on_gen_vertex_data = false;
}

GLuint Chunk::get_vbo() const { return m_vbo; }

size_t Chunk::get_vertex_sum() const {
    if (m_vertex_sum == 0) {
        Logger::warn("m_vertex_sum is 0");
    }
    return m_vertex_sum.load();
}

void Chunk::gen_phase_one() {
    m_generator = std::make_unique<ChunkGenerator>(*this);
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->assign_chunk_biome();
}

void Chunk::gen_phase_two(const std::array<const Chunk*, 8>& adj_chunks) {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->resolve_biome_adjacency_conflict(adj_chunks);
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
    m_generator->blend_heightmap_boundaries(neighbor_heightmap, neighbor_biome);
}

void Chunk::gen_phase_five() {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->generate_terrain_blocks();
}

void Chunk::gen_phase_six(
    const std::array<std::optional<std::vector<uint8_t>>, 4>& neighbor_block) {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->blend_surface_blocks_borders(neighbor_block);
}

void Chunk::gen_phase_seven() {
    if (!m_generator) {
        Logger::error("ChunkGenerator is Nullptr");
        return;
    }
    m_generator->generate_vegetation();
    mark_dirty();
    m_generator = nullptr;
}

void Chunk::upload_to_gpu() {

    ASSERT(is_need_upload());
    if (m_vbo == 0) {
        glGenBuffers(1, &m_vbo);
    }
    std::lock_guard lk(m_vertexs_data_mutex);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexs_data.size() * sizeof(Vertex),
                 m_vertexs_data.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
std::vector<uint8_t>& Chunk::blocks() { return m_blocks; }
World& Chunk::world() { return m_world; }
} // namespace Cubed
