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
} // namespace Cubed