#pragma once
#include <glm/glm.hpp>

namespace Cubed {

namespace Math {

void extract_frustum_planes(const glm::mat4& mvp_matrix,
                            std::vector<glm::vec4>& planes);

float smootherstep(float edge0, float edge1, float x);
} // namespace Math

} // namespace Cubed