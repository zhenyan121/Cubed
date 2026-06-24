#pragma once
#include "Cubed/gameplay/chunk_pos.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <string_view>
namespace Cubed {
class ServerWorld;
class Session;
class ServerPlayer {
public:
    ServerPlayer(std::string_view name, std::string_view uuid,
                 ServerWorld& m_world, std::shared_ptr<Session> session);
    const glm::vec3& get_pos() const;
    const std::string& get_name() const;
    const std::string& get_uuid() const;
    std::shared_ptr<Session> get_session() const;
    void update_pos(float x, float y, float z);

private:
    std::string m_name;
    std::string m_uuid;
    glm::vec3 m_pos{0.0f};
    ServerWorld& m_world;
    ChunkPos m_last_chunk_pos{0, 0};
    std::shared_ptr<Session> m_session;
};
} // namespace Cubed
