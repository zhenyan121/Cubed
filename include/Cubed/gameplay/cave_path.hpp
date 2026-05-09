#pragma once

#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/tools/cubed_random.hpp"

#include <glm/glm.hpp>
#include <unordered_set>
namespace Cubed {

struct PathPoint {
    glm::vec3 pos;
    glm::vec3 tangent{0.0f, 0.0f, 1.0f};
    float rad_xz;
    float rad_y;
    PathPoint(const glm::vec3& p, float rx, float ry)
        : pos(p), rad_xz(rx), rad_y(ry) {}
    bool contains(const glm::vec3& other_pos) const {
        glm::vec3 to_point = other_pos - pos;

        glm::vec3 world_up(0.0f, 1.0f, 0.0f);

        glm::vec3 right = glm::normalize(glm::cross(tangent, world_up));

        if (glm::length(right) < 0.001f) {
            glm::vec3 alt_up(1.0f, 0.0f, 0.0f);
            right = glm::normalize(glm::cross(tangent, alt_up));
        }

        glm::vec3 up = glm::normalize(glm::cross(right, tangent));

        float horizontal_dist = glm::dot(to_point, right);
        float vertical_dist = glm::dot(to_point, up);

        float a = rad_xz;
        float b = rad_y;
        if (a <= 0.0f || b <= 0.0f)
            return false;

        float check = (horizontal_dist * horizontal_dist) / (a * a) +
                      (vertical_dist * vertical_dist) / (b * b);
        return check <= 1.0f;
    }
};

class CavePath {
public:
    CavePath(unsigned int world_seed, int path_id, const glm::vec3& start_pos);
    const std::vector<PathPoint>& points() const;
    void clear_chunk(const ChunkPos& pos);
    bool is_finished() const;

    static float& radius_xz_min();
    static float& radius_xz_max();
    static float& radius_y_min();
    static float& radius_y_max();
    static float& delta_angle_min();
    static float& delta_angle_max();
    static int& step_min();
    static int& step_max();

private:
    static inline float m_radius_xz_min = 5.0f;
    static inline float m_radius_xz_max = 15.0f;
    static inline float m_radius_y_min = 4.0f;
    static inline float m_radius_y_max = 10.0f;
    static inline float m_delta_angle_min = -5.0f;
    static inline float m_delta_angle_max = 5.0f;
    static inline int m_step_min = 10;
    static inline int m_step_max = 400;

    int m_path_id = 0;
    unsigned int m_seed = 0;
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    int m_step = 0;
    float m_step_len = 1.0f;
    PathPoint m_start_path_point{{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f};
    Random m_random;

    std::vector<PathPoint> m_points;
    std::unordered_set<ChunkPos, ChunkPos::Hash> m_pending_chunks;
    void collect_path_points();
    void precompute_chunk_coverage();
};
} // namespace Cubed