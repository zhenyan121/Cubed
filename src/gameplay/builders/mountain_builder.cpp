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
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int height = static_cast<int>(m_heightmap[x][z]);
            for (int y = 5; y <= height; y++) {
                m_blocks[Chunk::index(x, y, z)] = 3;
            }
        }
    }
}

void MountainBuilder::build_vegetation() { fill_water(); }

ChunkGenerator& MountainBuilder::get_chunk_generator() {
    return m_chunk_generator;
};

} // namespace Cubed