#pragma once

#include <glm/glm.hpp>

namespace Cubed {

class ServerChunk;

struct TreeStructNode {
    glm::ivec3 offset{0, 0, 0};
    unsigned id = 0;
};

bool build_tree(ServerChunk& chunk, const glm::ivec3& pos);

} // namespace Cubed
