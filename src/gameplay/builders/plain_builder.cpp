#include "Cubed/gameplay/builders/plain_builder.hpp"

#include "Cubed/gameplay/chunk_generator.hpp"
#include "Cubed/gameplay/server_chunk.hpp"
namespace Cubed {
PlainBuilder::PlainBuilder(ChunkGenerator& chunk_generator)
    : m_chunk_generator(chunk_generator) {}

void PlainBuilder::build_biome() {
    BiomeBuilder::build_bottom();
    build_blocks();
};

void PlainBuilder::build_blocks() {
    auto& m_chunk = m_chunk_generator.chunk();
    auto& m_blocks = m_chunk.blocks();
    auto& m_heightmap = m_chunk.heightmap();
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int height = static_cast<int>(m_heightmap[x][z]);
            for (int y = 5; y < height - 5; y++) {
                m_blocks[ServerChunk::index(x, y, z)] = 3;
            }
            for (int y = height - 5; y < height; y++) {
                m_blocks[ServerChunk::index(x, y, z)] = 2;
            }
            m_blocks[ServerChunk::index(x, height, z)] = 1;
        }
    }
}

void PlainBuilder::build_vegetation() { place_grass(); }

ChunkGenerator& PlainBuilder::get_chunk_generator() {
    return m_chunk_generator;
};

} // namespace Cubed