#include "Cubed/gameplay/server_player.hpp"

#include "Cubed/gameplay/server_world.hpp"
namespace Cubed {
ServerPlayer::ServerPlayer(std::string_view name, std::string_view uuid,
                           ServerWorld& world, std::shared_ptr<Session> session,
                           TickType gametick)
    : m_name(name), m_uuid(uuid), m_world(world), m_session(session),
      m_last_gametick(gametick) {}
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
        m_last_chunk_pos = chunk_pos;
    }
}

void ServerPlayer::update_sync_gametick(TickType gametick) {
    m_last_gametick = gametick;
}
bool ServerPlayer::is_disconnect(TickType current_gametick) const {
    if (current_gametick - m_last_gametick > TIMEOUT) {
        return true;
    }
    return false;
}

int ServerPlayer::task_id() const { return m_chunk_task_id.load(); }
void ServerPlayer::task_id(int id) { m_chunk_task_id = id; }

bool ServerPlayer::has_player(ChunkPos pos) const {
    std::shared_lock lock(m_chunk_pos_mutex);
    return m_player_chunk_pos_set.find(pos) != m_player_chunk_pos_set.end();
}
void ServerPlayer::update_chunk_set(const ChunkPosSet& set) {
    std::lock_guard lock(m_chunk_pos_mutex);
    m_player_chunk_pos_set.clear();
    m_player_chunk_pos_set.insert(set.begin(), set.end());
}

const ServerPlayer::ChunkPosSet& ServerPlayer::get_chunk_pos_set() const {
    std::shared_lock lock(m_chunk_pos_mutex);
    return m_player_chunk_pos_set;
}

ServerPlayer::ChunkPosSet& ServerPlayer::get_chunk_pos_set() {
    std::lock_guard lock(m_chunk_pos_mutex);
    return m_player_chunk_pos_set;
}
} // namespace Cubed