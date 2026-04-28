#include "Cubed/tools/math_tools.hpp"

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

} // namespace Math

} // namespace Cubed