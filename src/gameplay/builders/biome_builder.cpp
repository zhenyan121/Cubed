#include "Cubed/gameplay/builders/biome_builder.hpp"

#include "Cubed/gameplay/chunk.hpp"
#include "Cubed/gameplay/chunk_generator.hpp"
namespace Cubed {
void BiomeBuilder::build_bottom() {
    ChunkGenerator& chunk_generator = get_chunk_generator();
    Chunk& chunk = chunk_generator.chunk();
    auto& m_blocks = chunk.blocks();
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < CHUCK_SIZE; z++) {
                m_blocks[Chunk::get_index(x, y, z)] = 3;
            }
        }
    }
}
void BiomeBuilder::fill_water() {
    ChunkGenerator& chunk_generator = get_chunk_generator();
    Chunk& chunk = chunk_generator.chunk();
    auto& m_blocks = chunk.blocks();
    auto& neighbor = chunk_generator.neighbor_biome();
    auto& heightmap = chunk.heightmap();
    for (int i = 0; i < 8; i++) {
        if (neighbor[i] == BiomeType::RIVER) {
            for (int x = 0; x < SIZE_X; x++) {
                for (int z = 0; z < SIZE_Z; z++) {
                    if (heightmap[x][z] >= SEA_LEVEL) {
                        continue;
                    }
                    int height = heightmap[x][z];
                    for (int y = height; y < SEA_LEVEL; y++) {
                        m_blocks[Chunk::get_index(x, y, z)] = 7;
                    }
                }
            }
            return;
        }
    }
}
} // namespace Cubed