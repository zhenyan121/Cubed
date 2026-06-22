#include "Cubed/gameplay/vertex_data.hpp"

#include "Cubed/gameplay/world.hpp"

namespace Cubed {
VertexData::VertexData(World& world) : m_world(world) {}
VertexData::~VertexData() {
    if (m_vbo != 0) {
        m_world.push_delete_vbo(m_vbo);
    }
    if (m_vao != 0) {
        m_world.push_delete_vao(m_vao);
    }
}
VertexData::VertexData(VertexData&& o) noexcept
    : m_vertices(std::move(o.m_vertices)), m_vbo(o.m_vbo), m_vao(o.m_vao),
      m_sum(o.m_sum.load()), m_world(o.m_world) {
    o.m_vbo = 0;
    o.m_sum = 0;
    o.m_vao = 0;
}
VertexData& VertexData::operator=(VertexData&& o) noexcept {
    m_vbo = o.m_vbo;
    o.m_vbo = 0;
    m_sum = o.m_sum.load();
    o.m_sum = 0;
    m_vertices = std::move(o.m_vertices);
    m_vao = o.m_vao;
    o.m_vao = 0;
    return *this;
}
void VertexData::upload() {
    if (m_vertices.size() == 0) {
        return;
    }
    if (m_vao == 0) {
        glGenVertexArrays(1, &m_vao);
    }
    if (m_vbo == 0) {
        glGenBuffers(1, &m_vbo);
    }
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex3D),
                 m_vertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, s));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, layer));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, nx));
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, roughness));
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, tx));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Release memory
    m_vertices.clear();
}
void VertexData::update_sum() { m_sum = m_vertices.size(); }
} // namespace Cubed