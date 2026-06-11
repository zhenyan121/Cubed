#include "Cubed/tools/math_tools.hpp"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
namespace Cubed {

namespace Math {

void extract_frustum_planes(const glm::mat4& mvp_matrix,
                            std::vector<glm::vec4>& planes) {
    if (planes.size() != 6) {
        planes.resize(6);
    }

    const float* m = glm::value_ptr(mvp_matrix);

    // left plane
    planes[0] =
        glm::vec4(m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12]);
    // right plane
    planes[1] =
        glm::vec4(m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]);
    // bottom plane
    planes[2] =
        glm::vec4(m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13]);
    // top plane
    planes[3] =
        glm::vec4(m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]);
    // near plane
    planes[4] =
        glm::vec4(m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]);
    // far plane
    planes[5] =
        glm::vec4(m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]);

    for (auto& p : planes) {
        p = glm::normalize(p);
    }
}

float smootherstep(float edge0, float edge1, float x) {

    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    return x * x * x * (x * (6.0f * x - 15.0f) + 10.0f);
}

bool is_aabb_in_frustum(const glm::vec3& center, const glm::vec3& half_extents,
                        const std::vector<glm::vec4>& planes) {
    for (const auto& plane : planes) {
        // distance
        float d = glm::dot(glm::vec3(plane), center) + plane.w;
        float r = half_extents.x * std::abs(plane.x) +
                  half_extents.y * std::abs(plane.y) +
                  half_extents.z * std::abs(plane.z);
        if (d + r < 0) {
            return false;
        }
    }
    return true;
}

} // namespace Math

} // namespace Cubed