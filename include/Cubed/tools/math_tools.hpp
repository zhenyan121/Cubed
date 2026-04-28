#pragma once
#include <glm/glm.hpp>

namespace Cubed {

namespace Math {
void extract_frustum_planes(const glm::mat4& mvp_matrix,
                            std::vector<glm::vec4>& planes);
}

} // namespace Cubed