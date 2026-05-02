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

    // Generate Biome
    void assign_chunk_biome();
    // Adjust Biome
    void resolve_biome_adjacency_conflict(
        const std::array<const Chunk*, 4>& adj_chunks);
    // Generate Heightmap
    void generate_heightmap();
    // Adjust Height
    void blend_heightmap_boundaries(
        const std::array<std::optional<HeightMapArray>, 4>& neighbor_heightmap);
    // Generate Block
    void generate_terrain_blocks();
    // Adjust Block;
    void blend_surface_blocks_borders(
        const std::array<std::optional<std::vector<uint8_t>>, 4>&
            neighbor_block);
    // Generate Structure
    void generate_vegetation();

    Chunk& chunk();
    Random& random();
    bool neighbor_river() const;

private:
    static inline std::atomic<bool> is_init{false};
    static inline unsigned m_generator_seed{0};
    static inline std::atomic<bool> is_seed_change{false};
    Chunk& m_chunk;
    Random m_random;
    std::array<BiomeType, 4> neighbor_biome{BiomeType::NONE, BiomeType::NONE,
                                            BiomeType::NONE, BiomeType::NONE};
    std::unique_ptr<BiomeBuilder> m_biome_builder{nullptr};
    bool is_neighbor_river = false;

    void make_biome_builder();
};

} // namespace Cubed