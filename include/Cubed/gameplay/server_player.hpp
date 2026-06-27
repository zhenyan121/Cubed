#pragma once
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/game_time.hpp"

#include <atomic>
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
                 ServerWorld& m_world, std::shared_ptr<Session> session,
                 TickType gametick);
    const glm::vec3& get_pos() const;
    const std::string& get_name() const;
    const std::string& get_uuid() const;
    std::shared_ptr<Session> get_session() const;
    void update_pos(float x, float y, float z);
    void update_sync_gametick(TickType gametick);
    bool is_disconnect(TickType current_gametick) const;
    int task_id() const;
    void task_id(int id);

private:
    static constexpr TickType TIMEOUT = 200;
    std::string m_name;
    std::string m_uuid;
    glm::vec3 m_pos{0.0f};
    ServerWorld& m_world;
    ChunkPos m_last_chunk_pos{0, 0};
    std::shared_ptr<Session> m_session;
    std::atomic<TickType> m_last_gametick{0};
    std::atomic<int> m_chunk_task_id{0};
};
} // namespace Cubed
