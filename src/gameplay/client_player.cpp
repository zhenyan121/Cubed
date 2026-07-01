#include "Cubed/gameplay/client_player.hpp"

#include "Cubed/config.hpp"
#include "Cubed/debug_collector.hpp"
#include "Cubed/gameplay/client_world.hpp"

namespace Cubed {
ClientPlayer::ClientPlayer(ClientWorld& world) : m_world(world) {}
ClientPlayer::~ClientPlayer() {}

AABB ClientPlayer::get_aabb(const glm::vec3& pos) {
    float half_width = M_SIZE.x / 2.0f;
    float half_depth = M_SIZE.z / 2.0f;

    glm::vec3 min{pos.x - half_width, pos.y, pos.z - half_depth};

    glm::vec3 max{pos.x + half_width, pos.y + M_SIZE.y, pos.z + half_depth};

    return AABB{min, max};
}
const glm::vec3& ClientPlayer::get_front() const { return m_front; }

const Gait& ClientPlayer::get_gait() const { return m_gait; }

const std::optional<LookBlock>& ClientPlayer::get_look_block_pos() const {
    return m_look_block;
}
glm::vec3 ClientPlayer::get_player_pos() const {

    std::shared_lock lock(m_player_pos_mutex);
    return m_player_pos;
}

const MoveState& ClientPlayer::get_move_state() const { return m_move_state; }

bool ClientPlayer::ray_cast(const glm::vec3& start, const glm::vec3& front,
                            glm::ivec3& block_pos, glm::vec3& normal,
                            float distance) {
    glm::vec3 dir = glm::normalize(front);
    // float step = 0.1f;
    glm::ivec3 cur = glm::floor(start);
    int ix = cur.x;
    int iy = cur.y;
    int iz = cur.z;
    // step direction
    int step_x = (dir.x > 0) ? 1 : ((dir.x < 0) ? -1 : 0);
    int step_y = (dir.y > 0) ? 1 : ((dir.y < 0) ? -1 : 0);
    int step_z = (dir.z > 0) ? 1 : ((dir.z < 0) ? -1 : 0);

    static const float INF = std::numeric_limits<float>::infinity();

    float t_delta_x = (dir.x != 0) ? std::fabs(1.0f / dir.x) : INF;
    float t_delta_y = (dir.y != 0) ? std::fabs(1.0f / dir.y) : INF;
    float t_delta_z = (dir.z != 0) ? std::fabs(1.0f / dir.z) : INF;

    float t_max_x, t_max_y, t_max_z;

    if (dir.x > 0) {
        t_max_x = (static_cast<float>(ix) + 1.0f - start.x) / dir.x;
    } else if (dir.x < 0) {
        t_max_x = (start.x - static_cast<float>(ix)) / (-dir.x);
    } else {
        t_max_x = INF;
    }

    if (dir.y > 0) {
        t_max_y = (static_cast<float>(iy) + 1.0f - start.y) / dir.y;
    } else if (dir.y < 0) {
        t_max_y = (start.y - static_cast<float>(iy)) / (-dir.y);
    } else {
        t_max_y = INF;
    }

    if (dir.z > 0) {
        t_max_z = (static_cast<float>(iz) + 1.0f - start.z) / dir.z;
    } else if (dir.z < 0) {
        t_max_z = (start.z - static_cast<float>(iz)) / (-dir.z);
    } else {
        t_max_z = INF;
    }
    float t = 0.0f;
    normal = glm::vec3(0.0f, 0.0f, 0.0f);
    while (t <= distance) {
        if (m_world.is_solid(glm::ivec3(ix, iy, iz))) {
            block_pos = glm::ivec3(ix, iy, iz);
            return true;
        }

        if (t_max_x < t_max_y && t_max_x < t_max_z) {
            t = t_max_x;
            t_max_x += t_delta_x;
            normal = glm::vec3(-step_x, 0.0f, 0.0f);
            ix += step_x;
        } else if (t_max_y < t_max_z) {
            t = t_max_y;
            t_max_y += t_delta_y;
            normal = glm::vec3(0.0f, -step_y, 0.0f);
            iy += step_y;
        } else {
            t = t_max_z;
            t_max_z += t_delta_z;
            normal = glm::vec3(0.0f, 0.0f, -step_z);
            iz += step_z;
        }
    }
    return false;
}

void ClientPlayer::change_mode(GameMode mode) {
    m_game_mode = mode;
    Logger::info("Change GameMode to {}", to_str(mode));
    if (mode == CREATIVE) {
        is_fly = false;
        m_gait = Gait::WALK;
    } else if (mode == SPECTATOR) {
        is_fly = true;
        m_gait = Gait::RUN;
        m_max_speed = m_max_run_speed;
    }
}
void ClientPlayer::hot_reload() {
    auto& config = Config::get();
    m_sensitivity =
        static_cast<float>(config.get<double>("player.mouse_sensitivity"));
}
void ClientPlayer::set_player_pos(const glm::vec3& pos) { m_player_pos = pos; }

void ClientPlayer::set_place_block(unsigned id) { m_place_block = id; }

void ClientPlayer::update(float delta_time) {

    update_move(delta_time);
    update_lookup_block();

    DebugCollector::get().report("player_pos",
                                 std::format("x: {:.2f} y: {:.2f} z: {:.2f}",
                                             m_player_pos.x, m_player_pos.y,
                                             m_player_pos.z));

    DebugCollector::get().report("speed",
                                 std::format("Speed: {:.2} m/s", m_xz_speed));
}
void ClientPlayer::update_player_move_state(int key, int action) {
    switch (key) {
    case GLFW_KEY_W:
        if (action == GLFW_PRESS) {
            m_move_state.forward = true;
        }
        if (action == GLFW_RELEASE) {
            m_move_state.forward = false;
            if (m_game_mode != SPECTATOR) {
                m_gait = Gait::WALK;
            }
        }
        break;
    case GLFW_KEY_S:
        if (action == GLFW_PRESS) {
            m_move_state.back = true;
        }
        if (action == GLFW_RELEASE) {
            m_move_state.back = false;
        }
        break;
    case GLFW_KEY_A:
        if (action == GLFW_PRESS) {
            m_move_state.left = true;
        }
        if (action == GLFW_RELEASE) {
            m_move_state.left = false;
        }
        break;
    case GLFW_KEY_D:
        if (action == GLFW_PRESS) {
            m_move_state.right = true;
        }
        if (action == GLFW_RELEASE) {
            m_move_state.right = false;
        }
        break;
    case GLFW_KEY_SPACE:
        if (action == GLFW_PRESS) {
            m_move_state.up = true;
            if (space_on) {
                if (m_game_mode == CREATIVE) {
                    is_fly = !is_fly ? true : false;
                    m_y_speed = 0.0f;
                }
                space_on = false;
                space_on_time = 0.0f;
            } else {
                space_on = true;
            }
        }
        if (action == GLFW_RELEASE) {
            m_move_state.up = false;
        }
        break;
    case GLFW_KEY_LEFT_SHIFT:
        if (action == GLFW_PRESS) {
            m_move_state.down = true;
        }
        if (action == GLFW_RELEASE) {
            m_move_state.down = false;
        }
        break;
    case GLFW_KEY_LEFT_CONTROL:
        if (action == GLFW_PRESS) {
            m_gait = Gait::RUN;
        }
        break;
    case GLFW_KEY_F4:
        if (action == GLFW_PRESS) {
            if (m_game_mode == CREATIVE) {
                change_mode(SPECTATOR);
            } else {
                change_mode(CREATIVE);
            }
        }
        break;
    }
}

void ClientPlayer::update_front_vec(float offset_x, float offset_y) {
    m_yaw += offset_x * m_sensitivity;
    m_pitch += offset_y * m_sensitivity;

    m_yaw = std::fmod(m_yaw, 360.0);

    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    m_front.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front.y = sin(glm::radians(m_pitch));
    m_front.z = -cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

    m_front = glm::normalize(m_front);
}

void ClientPlayer::update_direction() {
    m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 move_dir_front = glm::vec3(0.0f);
    glm::vec3 move_dir_right = glm::vec3(0.0f);
    glm::vec3 move_dir = glm::vec3(0.0f);
    if (m_move_state.forward) {
        move_dir_front += glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z));
    }
    if (m_move_state.back) {
        move_dir_front -= glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z));
    }
    if (m_move_state.left) {
        move_dir_right -= glm::normalize(glm::vec3(m_right.x, 0.0f, m_right.z));
    }
    if (m_move_state.right) {
        move_dir_right += glm::normalize(glm::vec3(m_right.x, 0.0f, m_right.z));
    }
    move_dir = move_dir_front + move_dir_right;

    if (glm::length(move_dir) > 0.001f) {
        direction = glm::normalize(move_dir);
    }
}

void ClientPlayer::update_lookup_block() {
    // calculate the block that is looked
    glm::ivec3 block_pos;
    glm::vec3 block_normal;
    if (ray_cast(
            glm::vec3(m_player_pos.x, (m_player_pos.y + 1.6f), m_player_pos.z),
            m_front, block_pos, block_normal)) {
        m_look_block = LookBlock{block_pos, glm::floor(block_normal)};
    } else {
        m_look_block = std::nullopt;
    }

    if (m_look_block != std::nullopt) {
        if (Input::get_input_state().mouse_state.left) {
            if (m_world.is_solid(m_look_block->pos)) {
                m_world.report_block_change(m_look_block->pos, 0);
            }
            Input::get_input_state().mouse_state.left = false;
        }
        if (Input::get_input_state().mouse_state.right) {
            glm::ivec3 near_pos = m_look_block->pos + m_look_block->normal;
            if (!m_world.is_solid(near_pos)) {
                AABB block_box = ClientWorld::get_block_aabb(near_pos);
                AABB player_box = get_aabb(get_player_pos());
                if (!player_box.intersects(block_box)) {
                    m_world.report_block_change(near_pos, m_place_block);
                }
            }
            Input::get_input_state().mouse_state.right = false;
        }
    }
}

void ClientPlayer::update_move(float delta_time) {
    // if frame rate less than 1 frame per second, don't update
    if (delta_time > 1.0f) {
        return;
    }
    // ensure the thread safe
    glm::vec3 player_pos;

    {
        std::shared_lock lock(m_player_pos_mutex);
        player_pos = m_player_pos;
    }

    if (m_game_mode != SPECTATOR) {
        if (m_gait == Gait::RUN) {
            m_max_speed = m_max_run_speed;
        }
        if (m_gait == Gait::WALK) {
            m_max_speed = m_max_walk_speed;
        }
    }

    if (space_on) {
        space_on_time += delta_time;
        if (space_on_time >= MAX_SPACE_ON_TIME) {
            space_on = false;
            space_on_time = 0.0f;
        }
    }

    // calculate speed
    if (m_move_state.forward || m_move_state.back || m_move_state.left ||
        m_move_state.right || m_move_state.up) {
        direction = glm::vec3(0.0f, 0.0f, 0.0f);
        m_xz_speed += m_acceleration * delta_time;
        if (m_xz_speed > m_max_speed) {
            m_xz_speed = m_max_speed;
        }
    } else {
        m_xz_speed += -m_deceleration * delta_time;
        if (m_xz_speed < 0) {
            m_xz_speed = 0;
            direction = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }

    update_direction();

    move_distance = {direction.x * m_xz_speed * delta_time, 0.0f,
                     direction.z * m_xz_speed * delta_time};

    if (is_fly) {
        if (m_move_state.up) {
            m_y_speed = m_fly_y_speed;
        }

        if (m_move_state.down) {
            m_y_speed = -m_fly_y_speed;
        }

        if (!m_move_state.down && !m_move_state.up) {
            m_y_speed = 0.0f;
        }
    } else {
        if (m_move_state.up && can_up) {
            m_y_speed = 7.5;
            can_up = false;
        }

        m_y_speed += -m_g * delta_time;
    }

    move_distance.y = m_y_speed * delta_time;
    // y
    update_y_move(player_pos);
    // x
    update_x_move(player_pos);

    update_z_move(player_pos);

    if (player_pos.y < -15.0f) {
        Logger::warn("y is tow low");
        player_pos += glm::vec3(1.0f, 100.0f, 1.0f);
    }

    {
        std::lock_guard lock(m_player_pos_mutex);
        m_player_pos = player_pos;
    }
    update_player_chunk();
}

void ClientPlayer::update_x_move(glm::vec3& player_pos) {
    player_pos.x += move_distance.x;
    if (m_game_mode == SPECTATOR) {
        return;
    }
    AABB player_box = get_aabb(player_pos);
    int minx = std::floor(player_box.min.x);
    int maxx = std::floor(player_box.max.x);
    int miny = std::floor(player_box.min.y);
    int maxy = std::floor(player_box.max.y);
    int minz = std::floor(player_box.min.z);
    int maxz = std::floor(player_box.max.z);

    for (int x = minx; x <= maxx; ++x) {
        for (int y = miny; y <= maxy; ++y) {
            for (int z = minz; z <= maxz; ++z) {
                glm::ivec3 block_pos{x, y, z};
                if (!m_world.can_pass_block(block_pos)) {
                    AABB block_box = ClientWorld::get_block_aabb(block_pos);
                    if (player_box.intersects(block_box)) {
                        m_gait = Gait::WALK;
                        player_pos.x -= move_distance.x;
                        return;
                    }
                }
            }
        }
    }
}

void ClientPlayer::update_y_move(glm::vec3& player_pos) {
    player_pos.y += move_distance.y;
    if (m_game_mode == SPECTATOR) {
        return;
    }
    AABB player_box = get_aabb(player_pos);
    int minx = std::floor(player_box.min.x);
    int maxx = std::floor(player_box.max.x);
    int miny = std::floor(player_box.min.y);
    int maxy = std::floor(player_box.max.y);
    int minz = std::floor(player_box.min.z);
    int maxz = std::floor(player_box.max.z);

    for (int x = minx; x <= maxx; ++x) {
        for (int y = miny; y <= maxy; ++y) {
            for (int z = minz; z <= maxz; ++z) {
                glm::ivec3 block_pos{x, y, z};
                if (!m_world.can_pass_block(block_pos)) {
                    AABB block_box = ClientWorld::get_block_aabb(block_pos);
                    if (player_box.intersects(block_box)) {
                        player_pos.y -= move_distance.y;
                        m_y_speed = 0.0f;
                        if (move_distance.y < 0) {
                            can_up = true;
                            is_fly = false;
                        }
                        return;
                    }
                }
            }
        }
    }
}

void ClientPlayer::update_z_move(glm::vec3& player_pos) {
    player_pos.z += move_distance.z;
    if (m_game_mode == SPECTATOR) {
        return;
    }
    AABB player_box = get_aabb(player_pos);
    int minx = std::floor(player_box.min.x);
    int maxx = std::floor(player_box.max.x);
    int miny = std::floor(player_box.min.y);
    int maxy = std::floor(player_box.max.y);
    int minz = std::floor(player_box.min.z);
    int maxz = std::floor(player_box.max.z);

    for (int x = minx; x <= maxx; ++x) {
        for (int y = miny; y <= maxy; ++y) {
            for (int z = minz; z <= maxz; ++z) {
                glm::ivec3 block_pos{x, y, z};
                if (!m_world.can_pass_block(block_pos)) {
                    AABB block_box = ClientWorld::get_block_aabb(block_pos);
                    if (player_box.intersects(block_box)) {
                        m_gait = Gait::WALK;
                        player_pos.z -= move_distance.z;
                        return;
                    }
                }
            }
        }
    }
}

void ClientPlayer::update_player_chunk() {
    float x, z;
    {
        std::shared_lock lock(m_player_pos_mutex);
        x = m_player_pos.x;
        z = m_player_pos.z;
    }
    ChunkPos chunk_pos = get_chunk_pos(x, z);
    float dist = distance2(chunk_pos, m_last_chunk_pos);
    if (dist > 2) {
        Logger::info("Player request new chunk");
        m_world.request_chunk();
        m_last_chunk_pos = chunk_pos;
    }
}

void ClientPlayer::update_scroll(double yoffset) {
    if (m_game_mode == SPECTATOR) {
        if (yoffset > 0) {
            if (m_max_speed < 500.0f) {
                m_max_speed += 1.0f;
            }
        } else {
            if (m_max_speed > 1.0f) {
                m_max_speed -= 1.0f;
            }
        }
    }
    if (m_game_mode == CREATIVE) {
        if (yoffset < 0) {
            m_place_block += 1;
            if (m_place_block >= BlockManager::sums()) {
                m_place_block = 1;
            }
        } else {
            m_place_block -= 1;
            if (m_place_block <= 0) {
                m_place_block = BlockManager::sums() - 1;
            }
        }
    }
}

void ClientPlayer::update_chunk_set(const ChunkPosSet& set) {
    std::lock_guard lock(m_chunk_pos_mutex);
    m_player_chunk_pos_set.clear();
    m_player_chunk_pos_set.insert(set.begin(), set.end());
}

const ClientPlayer::ChunkPosSet& ClientPlayer::get_chunk_pos_set() const {
    std::shared_lock lock(m_chunk_pos_mutex);
    return m_player_chunk_pos_set;
}

ClientPlayer::ChunkPosSet& ClientPlayer::get_chunk_pos_set() {
    std::lock_guard lock(m_chunk_pos_mutex);
    return m_player_chunk_pos_set;
}

float& ClientPlayer::max_walk_speed() { return m_max_walk_speed; }
float& ClientPlayer::max_run_speed() { return m_max_run_speed; }
float& ClientPlayer::max_speed() { return m_max_speed; }
float& ClientPlayer::acceleration() { return m_acceleration; }
float& ClientPlayer::deceleration() { return m_deceleration; }
float& ClientPlayer::g() { return m_g; }
float& ClientPlayer::fly_y_speed() { return m_fly_y_speed; }
unsigned ClientPlayer::place_block() const { return m_place_block; };
Gait& ClientPlayer::gait() { return m_gait; }
GameMode& ClientPlayer::game_mode() { return m_game_mode; }
const ClientWorld& ClientPlayer::get_world() const { return m_world; }

void ClientPlayer::set_uuid(std::string_view uuid) { m_uuid = uuid; }
const std::string& ClientPlayer::get_uuid() const { return m_uuid; }
const std::string& ClientPlayer::get_name() const { return m_name; }
void ClientPlayer::init(std::string_view name) { m_name = name; }
} // namespace Cubed