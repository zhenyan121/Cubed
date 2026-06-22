#include "Cubed/gameplay/river.path.hpp"
#include "Cubed/tools/cubed_hash.hpp"
#include "Cubed/tools/math_tools.hpp"

#include <algorithm>
namespace Cubed {
RiverPath::RiverPath(unsigned int chunk_seed, unsigned world_seed,
                     const glm::vec3& start_pos) {

    m_seed = HASH::combine_32(chunk_seed, world_seed);
    m_random.init(m_seed);

    m_yaw = m_random.random_float(0.0f, 360.0f);

    m_initial_yaw = m_yaw;
    m_pitch = 0.0f;
    m_start_path_point.pos = start_pos;
    m_start_path_point.rad_xz =
        m_random.random_float(m_radius_xz_min, m_radius_xz_max);
    m_start_path_point.rad_y =
        m_random.random_float(m_radius_y_min, m_radius_y_max);
    m_step = m_random.random_int(m_step_min, m_step_max);
    m_points.reserve(m_step + 1);
    m_points.push_back(m_start_path_point);
    collect_path_points();
}

void RiverPath::collect_path_points() {
    for (int i = 0; i < m_step; i++) {

        float dx = std::cos(glm::radians(m_pitch)) *
                   std::sin(glm::radians(m_yaw)) * m_step_len;
        float dy = std::sin(glm::radians(m_pitch)) * m_step_len;
        float dz = std::cos(glm::radians(m_pitch)) *
                   std::cos(glm::radians(m_yaw)) * m_step_len;

        m_points[i].tangent = glm::normalize(glm::vec3{dx, dy, dz});

        float t = Math::smootherstep(0, m_step - 1, i);

        float drad_xz = m_start_path_point.rad_xz * t;
        float drad_y = m_start_path_point.rad_y * t;
        drad_xz = std::max(drad_xz, 4.0f);
        drad_y = std::max(drad_y, 4.0f);
        m_points.emplace_back(m_points[i].pos + glm::vec3{dx, dy, dz}, drad_xz,
                              drad_y);

        m_yaw += m_random.random_float(m_delta_angle_min, m_delta_angle_max);
        m_yaw = std::clamp(m_yaw, m_initial_yaw - 10.0f, m_initial_yaw + 10.0f);
    }
    auto n = m_points.size();
    if (n >= 2) {
        m_points[n - 1].tangent = m_points[n - 2].tangent;
    }
}

const std::vector<PathPoint>& RiverPath::points() const { return m_points; }
float& RiverPath::radius_xz_min() { return m_radius_xz_min; }
float& RiverPath::radius_xz_max() { return m_radius_xz_max; }
float& RiverPath::radius_y_min() { return m_radius_y_min; }
float& RiverPath::radius_y_max() { return m_radius_y_max; }
float& RiverPath::delta_angle_min() { return m_delta_angle_min; }
float& RiverPath::delta_angle_max() { return m_delta_angle_max; }
int& RiverPath::step_min() { return m_step_min; }
int& RiverPath::step_max() { return m_step_max; }
float RiverPath::step_len() { return m_step_len; }
} // namespace Cubed