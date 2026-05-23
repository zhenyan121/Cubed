#pragma once
#include <glm/glm.hpp>
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