#include "Cubed/gameplay/builders/forest_builder.hpp"

#include "Cubed/gameplay/chunk.hpp"
#include "Cubed/gameplay/chunk_generator.hpp"
#include "Cubed/gameplay/tree.hpp"
namespace Cubed {
ForestBuilder::ForestBuilder(ChunkGenerator& chunk_generator)
    : m_chunk_generator(chunk_generator) {}

void ForestBuilder::build_biome() {
    BiomeBuilder::build_bottom();
    build_blocks();
};

void ForestBuilder::build_blocks() {
    auto& m_chunk = m_chunk_generator.chunk();
    auto& m_blocks = m_chunk.blocks();
    auto& m_heightmap = m_chunk.heightmap();
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int z = 0; z < CHUCK_SIZE; z++) {
            int height = static_cast<int>(m_heightmap[x][z]);
            for (int y = 5; y < height - 5; y++) {
                m_blocks[Chunk::get_index(x, y, z)] = 3;
            }
            for (int y = height - 5; y < height; y++) {
                m_blocks[Chunk::get_index(x, y, z)] = 2;
            }
            m_blocks[Chunk::get_index(x, height, z)] = 1;
        }
    }
}

void ForestBuilder::build_vegetation() {
    auto& m_chunk = m_chunk_generator.chunk();
    auto& m_heightmap = m_chunk.heightmap();
    auto& m_random = m_chunk_generator.random();
    std::array<int, SIZE_X> x_arr;
    std::iota(x_arr.begin(), x_arr.end(), 0);
    std::shuffle(x_arr.begin(), x_arr.end(), m_random.engine());
    std::array<int, SIZE_Z> z_arr;
    std::iota(z_arr.begin(), z_arr.end(), 0);
    std::shuffle(z_arr.begin(), z_arr.end(), m_random.engine());
    for (auto x : x_arr) {
        for (auto z : z_arr) {
            int y = static_cast<int>(m_heightmap[x][z]);
            if (m_random.random_bool(forest_params().tree_frequency) &&
                y >= SEA_LEVEL) {
                build_tree(m_chunk, {x, y, z});
            }
        }
    }
    fill_water();
}

ChunkGenerator& ForestBuilder::get_chunk_generator() {
    return m_chunk_generator;
};

} // namespace Cubed