#pragma once
#include "Cubed/primitive_data.hpp"

#include <atomic>
#include <glad/glad.h>
#include <vector>
namespace Cubed {
class World;
struct VertexData {
    std::vector<Vertex> m_vertices;
    GLuint m_vbo = 0;
    std::atomic<std::size_t> m_sum{0};
    World& m_world;
    VertexData(World& world);
    ~VertexData();
    VertexData(const VertexData&) = delete;
    VertexData(VertexData&&) noexcept;
    VertexData& operator=(const VertexData&) = delete;
    VertexData& operator=(VertexData&&) noexcept;

    void upload();
    void update_sum();
};
} // namespace Cubed
