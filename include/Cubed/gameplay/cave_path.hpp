#pragma once

#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/path_point.hpp"
#include "Cubed/tools/cubed_random.hpp"

#include <glm/glm.hpp>
#include <tbb/concurrent_hash_map.h>
namespace Cubed {

class CavePath {
    using ChunkPosSet =
        tbb::concurrent_hash_map<ChunkPos, bool, ChunkPos::TBBHash>;

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
    ChunkPosSet m_pending_chunks;
    void collect_path_points();
    void precompute_chunk_coverage();
};
} // namespace Cubed