#pragma once

#include <glm/glm.hpp>

namespace Cubed {

class Chunk;

struct TreeStructNode {
    glm::ivec3 offset{0, 0, 0};
    unsigned id = 0;
};

bool build_tree(Chunk& chunk, const glm::ivec3& pos);

}
