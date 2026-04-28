#pragma once

#include "Cubed/constants.hpp"
#include "Cubed/tools/cubed_random.hpp"

#include <atomic>
#include <optional>
namespace Cubed {

class Chunk;

class ChunkGenerator {
    static constexpr int SIZE_X = CHUCK_SIZE;
    static constexpr int SIZE_Y = WORLD_SIZE_Y;
    static constexpr int SIZE_Z = CHUCK_SIZE;
    using HeightMapArray =
        std::array<std::array<float, CHUCK_SIZE>, CHUCK_SIZE>;

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

private:
    static inline std::atomic<bool> is_init{false};
    static inline unsigned m_generator_seed{0};
    static inline std::atomic<bool> is_seed_change{false};
    Chunk& m_chunk;
    Random m_random;
};

} // namespace Cubed