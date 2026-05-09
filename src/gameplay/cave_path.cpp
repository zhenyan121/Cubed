#include "Cubed/gameplay/cave_path.hpp"

#include "Cubed/constants.hpp"
#include "Cubed/tools/cubed_hash.hpp"
#include "Cubed/tools/math_tools.hpp"

#include <algorithm>
namespace Cubed {
CavePath::CavePath(unsigned int world_seed, int path_id,
                   const glm::vec3& start_pos) {
    m_path_id = path_id;
    m_seed = HASH::combine_32(world_seed, path_id);
    m_random.init(m_seed);
    m_yaw = m_random.random_float(0.0f, 360.0f);
    m_pitch = m_random.random_float(-10.0f, 10.0f);
    m_start_path_point.pos = start_pos;
    m_start_path_point.rad_xz =
        m_random.random_float(m_radius_xz_min, m_radius_xz_max);
    m_start_path_point.rad_y =
        m_random.random_float(m_radius_y_min, m_radius_y_max);
    m_step = m_random.random_int(m_step_min, m_step_max);
    m_points.reserve(m_step + 1);
    m_points.push_back(m_start_path_point);
    collect_path_points();
    precompute_chunk_coverage();
}

void CavePath::collect_path_points() {
    for (int i = 0; i < m_step; i++) {

        m_yaw = std::fmod(m_yaw, 360.0f);
        if (m_yaw < 0.0f)
            m_yaw += 360.0f;
        m_pitch = std::clamp(m_pitch, -90.0f, 90.0f);

        float dx = std::cos(glm::radians(m_pitch)) *
                   std::sin(glm::radians(m_yaw)) * m_step_len;
        float dy = std::sin(glm::radians(m_pitch)) * m_step_len;
        float dz = std::cos(glm::radians(m_pitch)) *
                   std::cos(glm::radians(m_yaw)) * m_step_len;

        m_points[i].tangent = glm::normalize(glm::vec3{dx, dy, dz});

        float t = Math::smootherstep(0, m_step - 1, i);

        float drad_xz = m_start_path_point.rad_xz * (1.0f - t);
        float drad_y = m_start_path_point.rad_y * (1.0f - t);
        drad_xz = std::max(drad_xz, 4.0f);
        drad_y = std::max(drad_y, 4.0f);
        m_points.emplace_back(m_points[i].pos + glm::vec3{dx, dy, dz}, drad_xz,
                              drad_y);

        m_yaw += m_random.random_float(m_delta_angle_min, m_delta_angle_max);
        m_pitch += m_random.random_float(m_delta_angle_min, m_delta_angle_max);
    }
    auto n = m_points.size();
    if (n >= 2) {
        m_points[n - 1].tangent = m_points[n - 2].tangent;
    }
}

void CavePath::precompute_chunk_coverage() {
    for (const auto& point : m_points) {
        float rad = point.rad_xz;
        const glm::vec3& center = point.pos;

        int min_cx =
            static_cast<int>(std::floor((center.x - rad) / CHUNK_SIZE));
        int max_cx =
            static_cast<int>(std::floor((center.x + rad) / CHUNK_SIZE));
        int min_cz =
            static_cast<int>(std::floor((center.z - rad) / CHUNK_SIZE));
        int max_cz =
            static_cast<int>(std::floor((center.z + rad) / CHUNK_SIZE));

        for (int cx = min_cx; cx <= max_cx; ++cx)
            for (int cz = min_cz; cz <= max_cz; ++cz)
                m_pending_chunks.insert({cx, cz});
    }
}

void CavePath::clear_chunk(const ChunkPos& pos) { m_pending_chunks.erase(pos); }
const std::vector<PathPoint>& CavePath::points() const { return m_points; }
bool CavePath::is_finished() const { return m_pending_chunks.empty(); }

float& CavePath::radius_xz_min() { return m_radius_xz_min; }
float& CavePath::radius_xz_max() { return m_radius_xz_max; }
float& CavePath::radius_y_min() { return m_radius_y_min; }
float& CavePath::radius_y_max() { return m_radius_y_max; }
float& CavePath::delta_angle_min() { return m_delta_angle_min; }
float& CavePath::delta_angle_max() { return m_delta_angle_max; }
int& CavePath::step_min() { return m_step_min; }
int& CavePath::step_max() { return m_step_max; }

} // namespace Cubed
