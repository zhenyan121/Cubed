#include "Cubed/gameplay/builders/biome_builder.hpp"

#include "Cubed/gameplay/chunk.hpp"
#include "Cubed/gameplay/chunk_generator.hpp"
namespace Cubed {
void BiomeBuilder::build_bottom() {
    ChunkGenerator& chunk_generator = get_chunk_generator();
    Chunk& chunk = chunk_generator.chunk();
    auto& m_blocks = chunk.blocks();
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                m_blocks[Chunk::index(x, y, z)] = 3;
            }
        }
    }
}
void BiomeBuilder::place_grass() {
    ChunkGenerator& chunk_generator = get_chunk_generator();
    Chunk& chunk = chunk_generator.chunk();
    auto& blocks = chunk.blocks();
    const auto& heightmap = chunk.get_heightmap();
    auto& random = chunk_generator.random();
    for (int x = 0; x < SIZE_X; ++x) {
        for (int z = 0; z < SIZE_Z; ++z) {
            int y = heightmap[x][z];
            BlockType top_id = blocks[Chunk::index(x, y, z)];
            if (top_id != 1) {
                continue;
            }
            if (blocks[Chunk::index(x, y + 1, z)] != 0) {
                continue;
            }
            if (random.random_bool(0.2)) {
                if (y + 1 < SIZE_Y) {
                    blocks[Chunk::index(x, y + 1, z)] = 9;
                }
            }
        }
    }
}

void BiomeBuilder::ocean_water_build() {
    ChunkGenerator& chunk_generator = get_chunk_generator();
    Chunk& chunk = chunk_generator.chunk();
    auto& blocks = chunk.blocks();
    const auto& heightmap = chunk.get_heightmap();

    for (int x = 0; x < SIZE_X; ++x) {
        for (int z = 0; z < SIZE_Z; ++z) {
            int height = heightmap[x][z];
            if (height <= SEA_LEVEL) {
                for (int y = height; y <= SEA_LEVEL; y++) {
                    blocks[Chunk::index(x, y, z)] = 7;
                }
            }
        }
    }
}

} // namespace Cubed