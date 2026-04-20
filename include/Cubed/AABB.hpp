#pragma once
#include <glm/glm.hpp>

namespace Cubed {


struct AABB {
    glm::vec3 min{0.0f, 0.0f, 0.0f};
    glm::vec3 max{0.0f, 0.0f, 0.0f};

    AABB(glm::vec3 min_point, glm::vec3 max_point):
        min(min_point),
        max(max_point)
    {

    }

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

}