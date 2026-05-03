#include "Cubed/gameplay/tree.hpp"

#include "Cubed/gameplay/chunk.hpp"

#include <array>

namespace Cubed {

using glm::ivec3;

static constexpr std::array<TreeStructNode, 62> TREE{{
    {{0, 1, 0}, 5},   {{0, 2, 0}, 5},   {{0, 3, 0}, 5},  {{0, 4, 0}, 5},
    {{0, 5, 0}, 5},   {{0, 6, 0}, 6},   {{0, 5, 1}, 6},  {{1, 5, 0}, 6},
    {{0, 5, -1}, 6},  {{-1, 5, 0}, 6},  {{1, 5, 1}, 6},  {{1, 5, -1}, 6},
    {{-1, 5, -1}, 6}, {{-1, 5, 1}, 6},  {{0, 4, 1}, 6},  {{1, 4, 0}, 6},
    {{0, 4, -1}, 6},  {{-1, 4, 0}, 6},  {{1, 4, 1}, 6},  {{1, 4, -1}, 6},
    {{-1, 4, -1}, 6}, {{-1, 4, 1}, 6},  {{0, 4, 2}, 6},  {{2, 4, 0}, 6},
    {{0, 4, -2}, 6},  {{-2, 4, 0}, 6},  {{2, 4, 2}, 6},  {{2, 4, -2}, 6},
    {{-2, 4, -2}, 6}, {{-2, 4, 2}, 6},  {{1, 4, 2}, 6},  {{2, 4, 1}, 6},
    {{-1, 4, 2}, 6},  {{2, 4, -1}, 6},  {{1, 4, -2}, 6}, {{-2, 4, 1}, 6},
    {{-1, 4, -2}, 6}, {{-2, 4, -1}, 6}, {{0, 3, 1}, 6},  {{1, 3, 0}, 6},
    {{0, 3, -1}, 6},  {{-1, 3, 0}, 6},  {{1, 3, 1}, 6},  {{1, 3, -1}, 6},
    {{-1, 3, -1}, 6}, {{-1, 3, 1}, 6},  {{0, 3, 2}, 6},  {{2, 3, 0}, 6},
    {{0, 3, -2}, 6},  {{-2, 3, 0}, 6},  {{2, 3, 2}, 6},  {{2, 3, -2}, 6},
    {{-2, 3, -2}, 6}, {{-2, 3, 2}, 6},  {{1, 3, 2}, 6},  {{2, 3, 1}, 6},
    {{-1, 3, 2}, 6},  {{2, 3, -1}, 6},  {{1, 3, -2}, 6}, {{-2, 3, 1}, 6},
    {{-1, 3, -2}, 6}, {{-2, 3, -1}, 6},
}};

bool build_tree(Chunk& chunk, const glm::ivec3& pos) {
    auto& block = chunk.get_chunk_blocks();

    if (block[Chunk::get_index(pos)] != 1) {
        Logger::info("Root is not Grass Block");
        return false;
    }
    for (const auto& d : TREE) {
        auto tree_node = pos + d.offset;
        int x = tree_node.x;
        int y = tree_node.y;
        int z = tree_node.z;
        if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= WORLD_SIZE_Y ||
            z >= CHUNK_SIZE) {
            return false;
        }
        if (block[Chunk::get_index(tree_node)] != 0) {
            return false;
        }
    }
    for (const auto& d : TREE) {
        auto tree_node = pos + d.offset;
        chunk.set_chunk_block(Chunk::get_index(tree_node), d.id);
    }
    return true;
}

} // namespace Cubed