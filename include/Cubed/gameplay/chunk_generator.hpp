#pragma once

#include "Cubed/constants.hpp"
#include "Cubed/gameplay/biome.hpp"
#include "Cubed/gameplay/builders/biome_builder.hpp"
#include "Cubed/tools/cubed_random.hpp"

#include <atomic>
#include <memory>
#include <optional>
namespace Cubed {

class Chunk;

class ChunkGenerator {
public:
    ChunkGenerator(Chunk& chunk);

    static void init();
    static void reload();
    static const unsigned& seed();
    static void seed(unsigned s);
    unsigned chunk_seed() const;
    // Generate Biome
    void assign_chunk_biome();
    // Adjust Biome
    void resolve_biome_adjacency_conflict(
        const std::array<const Chunk*, 8>& adj_chunks);
    // Generate Heightmap
    void generate_heightmap();
    // Adjust Height
    void blend_heightmap_boundaries(
        const std::array<std::optional<HeightMapArray>, 8>& neighbor_heightmap,
        const std::array<BiomeType, 8>& neighbor_biome);
    // Generate Block
    void generate_terrain_blocks();
    // Adjust Block;
    void blend_surface_blocks_borders(
        const std::array<std::optional<std::vector<uint8_t>>, 4>&
            neighbor_block);
    // Generate Structure
    void generate_vegetation();
    BiomeType get_biome_at(float world_x, float world_z);
    Chunk& chunk();
    Random& random();
    const std::array<BiomeType, 8>& neighbor_biome() const;

private:
    static inline std::atomic<bool> is_init{false};
    static inline unsigned m_generator_seed{0};
    static inline std::atomic<bool> is_seed_change{false};
    Chunk& m_chunk;
    Random m_random;
    std::unique_ptr<BiomeBuilder> m_biome_builder{nullptr};
    bool is_cur_chunk_ins = false;
    std::array<BiomeType, 8> m_neighbor_biome;
    unsigned m_chunk_seed = 0;

    void make_biome_builder();
    void generate_cave();
};

} // namespace Cubed