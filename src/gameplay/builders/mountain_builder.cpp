#include "Cubed/gameplay/builders/mountain_builder.hpp"

#include "Cubed/gameplay/chunk.hpp"
#include "Cubed/gameplay/chunk_generator.hpp"
namespace Cubed {
MountainBuilder::MountainBuilder(ChunkGenerator& chunk_generator)
    : m_chunk_generator(chunk_generator) {}

void MountainBuilder::build_biome() {
    BiomeBuilder::build_bottom();
    build_blocks();
};

void MountainBuilder::build_blocks() {
    auto& m_chunk = m_chunk_generator.chunk();
    auto& m_blocks = m_chunk.blocks();
    auto& m_heightmap = m_chunk.heightmap();
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {
            int height = static_cast<int>(m_heightmap[x][z]);
            for (int y = 5; y < height - 5; y++) {
                m_blocks[Chunk::get_index(x, y, z)] = 3;
            }
            for (int y = height - 5; y <= height - 1; y++) {
                if (y > 110) {
                    m_blocks[Chunk::get_index(x, y, z)] = 3;
                } else {
                    m_blocks[Chunk::get_index(x, y, z)] = 2;
                }
            }
            if (height > 110) {
                m_blocks[Chunk::get_index(x, height - 1, z)] = 3;
            } else {
                m_blocks[Chunk::get_index(x, height - 1, z)] = 1;
            }
        }
    }
}

void MountainBuilder::build_vegetation() {
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

ChunkGenerator& MountainBuilder::get_chunk_generator() {
    return m_chunk_generator;
};

} // namespace Cubed