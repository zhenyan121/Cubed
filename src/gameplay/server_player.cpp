#include "Cubed/gameplay/server_player.hpp"

#include "Cubed/gameplay/server_world.hpp"
namespace Cubed {
ServerPlayer::ServerPlayer(std::string_view name, std::string_view uuid,
                           ServerWorld& world, std::shared_ptr<Session> session)
    : m_name(name), m_uuid(uuid), m_world(world), m_session(session) {}
const glm::vec3& ServerPlayer::get_pos() const { return m_pos; }
const std::string& ServerPlayer::get_name() const { return m_name; }
const std::string& ServerPlayer::get_uuid() const { return m_uuid; }
std::shared_ptr<Session> ServerPlayer::get_session() const { return m_session; }
void ServerPlayer::update_pos(float x, float y, float z) {
    m_pos = glm::vec3{x, y, z};
    ChunkPos chunk_pos = get_chunk_pos(x, z);
    float dist = distance2(chunk_pos, m_last_chunk_pos);
    if (dist > 2) {
        m_world.need_gen(m_uuid);
    }
}
} // namespace Cubed