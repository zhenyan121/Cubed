#include "Cubed/gameplay/builders/desert_builder.hpp"

#include "Cubed/gameplay/chunk.hpp"
#include "Cubed/gameplay/chunk_generator.hpp"
namespace Cubed {
DesertBuilder::DesertBuilder(ChunkGenerator& chunk_generator)
    : m_chunk_generator(chunk_generator) {}

void DesertBuilder::build_biome() {
    BiomeBuilder::build_bottom();
    build_blocks();
};

void DesertBuilder::build_blocks() {
    auto& m_chunk = m_chunk_generator.chunk();
    auto& m_blocks = m_chunk.blocks();
    auto& m_heightmap = m_chunk.heightmap();
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {
            int height = static_cast<int>(m_heightmap[x][z]);
            for (int y = 5; y < height - 5; y++) {
                m_blocks[Chunk::get_index(x, y, z)] = 3;
            }

            for (int y = height - 5; y <= height; y++) {
                m_blocks[Chunk::get_index(x, y, z)] = 4;
            }
        }
    }
}

void DesertBuilder::build_vegetation() {
    auto& m_chunk = m_chunk_generator.chunk();
    auto& m_blocks = m_chunk.blocks();
    auto& m_heightmap = m_chunk.heightmap();
    if (m_chunk_generator.neighbor_river()) {
        for (int x = 0; x < SIZE_X; x++) {
            for (int z = 0; z < SIZE_Z; z++) {
                int height = static_cast<int>(m_heightmap[x][z]);
                if (height >= SEA_LEVEL) {
                    continue;
                }
                for (int y = height + 1; y < SEA_LEVEL; y++) {
                    m_blocks[Chunk::get_index(x, y, z)] = 7;
                }
            }
        }
    }
}

ChunkGenerator& DesertBuilder::get_chunk_generator() {
    return m_chunk_generator;
};

} // namespace Cubed