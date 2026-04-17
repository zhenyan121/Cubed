#pragma once
#include <glm/glm.hpp>
namespace Math {
    void extract_frustum_planes(const glm::mat4& mvp_matrix, std::vector<glm::vec4>& planes);
    int get_interpolated_height(float world_x, float world_z, float temp, float humid);
}