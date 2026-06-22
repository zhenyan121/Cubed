#include "Cubed/gameplay/chunk.hpp"

#include "Cubed/gameplay/world.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"

#include <utility>

namespace Cubed {
using OptionalBlockVectorArray =
    std::array<std::optional<std::vector<BlockType>>, 4>;
namespace {
// ────────────────────────────────────────────────────────────────────────────
// Face direction mapping
//   Original DIR[6]: {+Z,+X,-Z,-X,+Y,-Y}  => face index 0-5
//   Axis × direction => face:
//     axis=2(Z) dir=+1 => face 0   (+Z)
//     axis=0(X) dir=+1 => face 1   (+X)
//     axis=2(Z) dir=-1 => face 2   (-Z)
//     axis=0(X) dir=-1 => face 3   (-X)
//     axis=1(Y) dir=+1 => face 4   (+Y)
//     axis=1(Y) dir=-1 => face 5   (-Y)
// ────────────────────────────────────────────────────────────────────────────

inline int axis_dir_to_face(int axis, int dir) {
    // axis: 0=X 1=Y 2=Z
    // dir:  +1 or -1
    static const int TABLE[3][2] = {
        {3, 1}, // X: dir=-1->face3(-X), dir=+1->face1(+X)
        {5, 4}, // Y: dir=-1->face5(-Y), dir=+1->face4(+Y)
        {2, 0}, // Z: dir=-1->face2(-Z), dir=+1->face0(+Z)
    };
    return TABLE[axis][dir > 0 ? 1 : 0];
}

inline BlockType
get_block_safe(int lx, int ly, int lz, ChunkPos& chunk_pos,
               const std::vector<BlockType>& blocks,
               const OptionalBlockVectorArray& neighbor_block) {
    if (lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < WORLD_SIZE_Y && lz >= 0 &&
        lz < CHUNK_SIZE) {
        return blocks[Chunk::index(lx, ly, lz)];
    }

    // Out of bounds: check neighbors
    int world_x = lx + chunk_pos.x * CHUNK_SIZE;
    int world_z = lz + chunk_pos.z * CHUNK_SIZE;

    auto [nb_cx, nb_cz] = World::get_chunk_pos(world_x, world_z);

    const std::optional<std::vector<BlockType>>* nb = nullptr;
    if (nb_cx == chunk_pos.x + 1)
        nb = &neighbor_block[0];
    else if (nb_cx == chunk_pos.x - 1)
        nb = &neighbor_block[1];
    else if (nb_cz == chunk_pos.z + 1)
        nb = &neighbor_block[2];
    else if (nb_cz == chunk_pos.z - 1)
        nb = &neighbor_block[3];

    if (!nb || !nb->has_value())
        return 0; // Neighbor does not exist, treat as opaque

    int nbx = world_x - nb_cx * CHUNK_SIZE;
    int nby = ly;
    int nbz = world_z - nb_cz * CHUNK_SIZE;

    if (nbx < 0 || nby < 0 || nbz < 0 || nbx >= CHUNK_SIZE ||
        nby >= WORLD_SIZE_Y || nbz >= CHUNK_SIZE)
        return 0;

    int idx = Chunk::index(nbx, nby, nbz);
    if (static_cast<size_t>(idx) >= (*nb)->size()) {
        return 0;
    }

    return (**nb)[idx];
}
// Determine whether the face from cur_id looking towards neighbor_id should be
// culled (does not need to be rendered)
inline bool is_face_culled(BlockType cur_id, BlockType neighbor_id) {
    if (!BlockManager::is_transparent(neighbor_id))
        return true; // Neighbor is opaque, blocking
    // Neighbor transparency: same block type culls each other (e.g., water
    // adjacent to water does not render internal faces)
    if (neighbor_id == cur_id)
        return true;
    return false;
}

inline int choose_buf(BlockType id) {
    if (!BlockManager::is_transparent(id))
        return 0;
    if (BlockManager::is_discard(id))
        return 2;
    if (BlockManager::is_blend(id)) {
        return (id == 7) ? 4 : 3; // water=4, other blend=3
    }
    return 3; // fallback
}

} // namespace

Chunk::Chunk(World& world, ChunkPos chunk_pos, bool temp_chunk)
    : m_temp_chunk(temp_chunk), m_chunk_pos(chunk_pos), m_world(world) {
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
      m_conditions(other.m_conditions), m_info(std::move(other.m_info)) {}

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
    m_info = std::move(other.m_info);
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

ChunkInfo Chunk::get_info() const {
    if (m_gening) {
        return ChunkInfo{};
    }
    return m_info;
}
/*
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
                            World::get_chunk_pos(world_nx, world_nz);

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
                            BlockManager::roughness(cur_id),
                            TANGENTS[face][i][0],
                            TANGENTS[face][i][1],
                            TANGENTS[face][i][2]

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
*/
void Chunk::gen_vertices(const OptionalBlockVectorArray& neighbor_block) {

    // SIZE_X=SIZE_Z=CHUNK_SIZE=16, SIZE_Y=WORLD_SIZE_Y=256
    // Axis order: axis 0=X, 1=Y, 2=Z
    // Two slice dimensions of each axis
    const int DIMS[3] = {CHUNK_SIZE, WORLD_SIZE_Y, CHUNK_SIZE};

    // Maximum mask size: max(16*256, 16*16) = 4096
    static thread_local FaceKey mask[CHUNK_SIZE * WORLD_SIZE_Y];
    static thread_local bool visited[CHUNK_SIZE * WORLD_SIZE_Y];

    for (int axis = 0; axis < 3; axis++) {
        int u_axis = (axis + 1) % 3; // horizontal
        int v_axis = (axis + 2) % 3; // vertical

        int u = DIMS[u_axis];
        int v = DIMS[v_axis];
        int d = DIMS[axis]; // Depth along the normal axis

        for (int face_dir : {1, -1}) {
            int face_idx = axis_dir_to_face(axis, face_dir);

            for (int layer = 0; layer < d; layer++) {

                // ── 1. Build mask ──────────────────────────────────────────
                for (int vi = 0; vi < v; vi++) {
                    for (int ui = 0; ui < u; ui++) {
                        // Current cell local coordinates
                        int lpos[3];
                        lpos[axis] = layer;
                        lpos[u_axis] = ui;
                        lpos[v_axis] = vi;

                        // Neighbor (offset one cell along the normal direction)
                        int npos[3];
                        npos[axis] = layer + face_dir;
                        npos[u_axis] = ui;
                        npos[v_axis] = vi;

                        BlockType cur_id = get_block_safe(
                            lpos[0], lpos[1], lpos[2], m_chunk_pos, m_blocks,
                            neighbor_block);

                        // Air / cross plane are not involved in greedy meshing
                        if (cur_id == 0 ||
                            BlockManager::is_cross_plane(cur_id)) {
                            mask[vi * u + ui] = {};
                            continue;
                        }

                        BlockType nb_id = get_block_safe(
                            npos[0], npos[1], npos[2], m_chunk_pos, m_blocks,
                            neighbor_block);

                        if (is_face_culled(cur_id, nb_id)) {
                            mask[vi * u + ui] = {};
                        } else {
                            mask[vi * u + ui] = {cur_id, face_idx};
                        }
                    }
                }

                // ── 2. Greedy Merge ──────────────────────────────────────
                std::fill(visited, visited + u * v, false);

                for (int vi = 0; vi < v; vi++) {
                    for (int ui = 0; ui < u; ui++) {
                        if (visited[vi * u + ui])
                            continue;
                        FaceKey cur = mask[vi * u + ui];
                        if (!cur.valid())
                            continue;

                        // Extend width in the u direction
                        int w = 1;
                        while (ui + w < u && !visited[vi * u + (ui + w)] &&
                               mask[vi * u + (ui + w)] == cur) {
                            w++;
                        }

                        // Extend height in the v direction
                        int h = 1;
                        bool can_expand = true;
                        while (vi + h < v && can_expand) {
                            for (int k = 0; k < w; k++) {
                                int idx = (vi + h) * u + (ui + k);
                                if (visited[idx] || mask[idx] != cur) {
                                    can_expand = false;
                                    break;
                                }
                            }
                            if (can_expand)
                                h++;
                        }

                        // mark visited
                        for (int dv = 0; dv < h; dv++)
                            for (int du = 0; du < w; du++)
                                visited[(vi + dv) * u + (ui + du)] = true;

                        // output quad
                        emit_quad(axis, face_dir, layer, ui, vi, w, h, u_axis,
                                  v_axis, cur);
                    }
                }
            }
        }
    }

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < WORLD_SIZE_Y; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                BlockType id = m_blocks[index(x, y, z)];
                if (id != 0 && BlockManager::is_cross_plane(id)) {
                    int world_x = x + m_chunk_pos.x * CHUNK_SIZE;
                    int world_z = z + m_chunk_pos.z * CHUNK_SIZE;
                    gen_cross_plane_vertices(world_x, y, world_z, id);
                }
            }
        }
    }
}
void Chunk::emit_quad(int axis, int face_dir, int layer, int i, int j, int w,
                      int h, int u_axis, int v_axis, FaceKey key) {
    float axis_val = (float)(layer + (face_dir > 0 ? 1 : 0));
    float wx_base = (float)(m_chunk_pos.x * CHUNK_SIZE);
    float wz_base = (float)(m_chunk_pos.z * CHUNK_SIZE);

    // Offsets of the four corners along the u_axis/v_axis
    int su[4] = {0, w, w, 0};
    int sv[4] = {0, 0, h, h};

    // Each face's UV: directly read from the four corners of TEX_COORDS, then
    // scaled by w/h TEX_COORDS vertex order: 0=BL, 1=TL, 2=TR, 3=TR, 4=BR, 5=BL
    // (two triangles) Four unique corners correspond to indices: BL=0, TL=1,
    // TR=2, BR=4 Extract the UVs of the four corners from TEX_COORDS (unique
    // corners after removing duplicate vertices) Vertices 0,1,2,4 correspond to
    // BL, TL, TR, BR
    float u0 = TEX_COORDS[key.face][0][0]; // BL.u
    float v0 = TEX_COORDS[key.face][0][1]; // BL.v
    float u1 = TEX_COORDS[key.face][4][0]; // BR.u
    float v1 = TEX_COORDS[key.face][4][1]; // BR.v
    float u3 = TEX_COORDS[key.face][1][0]; // TL.u
    float v3 = TEX_COORDS[key.face][1][1]; // TL.v

    float du_u = u1 - u0; // Change in u when su increases (per block)
    float dv_u = v1 - v0;
    float du_v = u3 - u0; // Change in u when sv increases
    float dv_v = v3 - v0;

    float uvs[4][2] = {
        {u0, v0},                                     // (0,  0 )
        {u0 + du_u * (float)w, v0 + dv_u * (float)w}, // (w,  0 )
        {u0 + du_u * (float)w + du_v * (float)h,
         v0 + dv_u * (float)w + dv_v * (float)h},     // (w,  h )
        {u0 + du_v * (float)h, v0 + dv_v * (float)h}, // (0,  h )
    };

    int tri[6] = {0, 1, 2, 0, 2, 3};

    float pos[4][3];
    for (int c = 0; c < 4; c++) {
        pos[c][axis] = axis_val;
        pos[c][u_axis] = (float)(i + su[c]);
        pos[c][v_axis] = (float)(j + sv[c]);
        pos[c][0] += wx_base;
        pos[c][2] += wz_base;
    }

    float layer_id = (float)(key.block_id * 6 + key.face);
    float roughness = BlockManager::roughness(key.block_id);
    int buf = choose_buf(key.block_id);

    for (int vi = 0; vi < 6; vi++) {
        int c = tri[vi];
        Vertex3D vex = {
            pos[c][0],
            pos[c][1],
            pos[c][2],
            uvs[c][0],
            uvs[c][1],
            layer_id,
            NORMALS[key.face][0][0],
            NORMALS[key.face][0][1],
            NORMALS[key.face][0][2],
            roughness,
            TANGENTS[key.face][0][0],
            TANGENTS[key.face][0][1],
            TANGENTS[key.face][0][2],
        };
        m_vertex_data[buf].m_vertices.emplace_back(vex);
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
                BlockManager::roughness(id),
                CROSS_TANGENTS[face][i][0],
                CROSS_TANGENTS[face][i][1],
                CROSS_TANGENTS[face][i][2]

            };
            m_vertex_data[1].m_vertices.emplace_back(vex);
        }
    }
}

void Chunk::gen_chunk() {
    if (m_gening.exchange(true))
        return;
    m_gening = true;
    if (m_blocks.size() != 0) {
        Logger::warn(
            "Request Generator Chunk {} {} ,but the Blocks size is Not 0",
            m_chunk_pos.x, m_chunk_pos.z);
    }
    std::vector<Chunk> neighbor;
    for (int i = 0; i < 4; i++) {
        neighbor.emplace_back(m_world, m_chunk_pos + CHUNK_DIR[i], true);
    }
    for (auto& chunk : neighbor) {
        chunk.gen_phase_one();
        chunk.gen_phase_three();
        chunk.gen_phase_five();
        chunk.gen_phase_seven();
    }
    gen_phase_one();
    gen_phase_three();
    gen_phase_five();

    OptionalBlockVectorArray neightbor_blocks;
    for (int i = 0; i < 4; i++) {
        neightbor_blocks[i] = neighbor[i].get_chunk_blocks();
    }
    gen_phase_six(neightbor_blocks);
    gen_phase_seven();
    for (int i = 0; i < 4; i++) {
        neightbor_blocks[i] = neighbor[i].get_chunk_blocks();
    }
    gen_vertex_data(neightbor_blocks);

    // collect chunk info for debugging
    m_info.biome = m_biome;
    m_info.pos = m_chunk_pos;
    m_info.seed = m_seed;
    Random r(m_seed);
    unsigned first = r.engine()();
    m_info.first_random = first;
    r.init(m_seed);
    m_info.has_cave_start = r.random_bool(DEFAULT_CAVE_PROBABILITY);
    m_info.has_cave = m_has_cave;
}
// Logger::info("Cross Sum {}", m_cross_vertices_sum.load());

bool Chunk::is_temp_chunk() const { return m_temp_chunk.load(); }

bool& Chunk::has_cave() { return m_has_cave; }

} // namespace Cubed
