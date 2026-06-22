#pragma once

#include "Cubed/gameplay/path_point.hpp"
#include "Cubed/tools/cubed_random.hpp"

#include <glm/glm.hpp>
#include <tbb/concurrent_hash_map.h>

namespace Cubed {
class RiverPath {

public:
    RiverPath(unsigned int chunk_seed, unsigned world_seed,
              const glm::vec3& start_pos);
    const std::vector<PathPoint>& points() const;

    static float& radius_xz_min();
    static float& radius_xz_max();
    static float& radius_y_min();
    static float& radius_y_max();
    static float& delta_angle_min();
    static float& delta_angle_max();
    static int& step_min();
    static int& step_max();
    static float step_len();

private:
    static inline float m_radius_xz_min = 5.0f;
    static inline float m_radius_xz_max = 10.0f;
    static inline float m_radius_y_min = 4.0f;
    static inline float m_radius_y_max = 8.0f;
    static inline float m_delta_angle_min = -3.0f;
    static inline float m_delta_angle_max = 3.0f;
    static inline int m_step_min = 200;
    static inline int m_step_max = 400;
    static inline float m_step_len = 4.0f;
    unsigned int m_seed = 0;
    float m_yaw = 0.0f;
    float m_initial_yaw = 0.0f;
    float m_pitch = 0.0f;
    int m_step = 0;
    PathPoint m_start_path_point{{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f};
    Random m_random;

    std::vector<PathPoint> m_points;
    void collect_path_points();
};
} // namespace Cubed
