#include "Cubed/gameplay/vertex_data.hpp"

#include "Cubed/gameplay/world.hpp"

namespace Cubed {
VertexData::VertexData(World& world) : m_world(world) {}
VertexData::~VertexData() {
    if (m_vbo != 0) {
        m_world.push_delete_vbo(m_vbo);
    }
}
VertexData::VertexData(VertexData&& o) noexcept
    : m_vertices(std::move(o.m_vertices)), m_vbo(o.m_vbo),
      m_sum(o.m_sum.load()), m_world(o.m_world) {
    o.m_vbo = 0;
    o.m_sum = 0;
}
VertexData& VertexData::operator=(VertexData&& o) noexcept {
    m_vbo = o.m_vbo;
    o.m_vbo = 0;
    m_sum = o.m_sum.load();
    o.m_sum = 0;
    m_vertices = std::move(o.m_vertices);
    return *this;
}
void VertexData::upload() {
    if (m_vertices.size() == 0) {
        return;
    }
    if (m_vbo == 0) {
        glGenBuffers(1, &m_vbo);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex),
                 m_vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void VertexData::update_sum() { m_sum = m_vertices.size(); }
} // namespace Cubed