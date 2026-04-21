#include <Cubed/gameplay/player.hpp>

#include <Cubed/debug_collector.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>
#include <GLFW/glfw3.h>

namespace Cubed {

Player::Player(World& world, const std::string& name) :
    m_name(name),
    m_world(world)    
{

}
Player::~Player() {

}

AABB Player::get_aabb() const {
    float half_width = m_size.x / 2.0f;
    float half_depth = m_size.z / 2.0f;

    glm::vec3 min{
        m_player_pos.x - half_width,
        m_player_pos.y,
        m_player_pos.z - half_depth
    };

    glm::vec3 max {
        m_player_pos.x + half_width,
        m_player_pos.y + m_size.y,
        m_player_pos.z + half_depth
    };

    return AABB{min, max};

}

const glm::vec3& Player::get_front() const {
    return m_front;
}

const Gait& Player::get_gait() const {
    return m_gait;
}

const std::optional<LookBlock>& Player::get_look_block_pos() const {
    return m_look_block;
}

const glm::vec3& Player::get_player_pos() const {
    return m_player_pos;
}

const MoveState& Player::get_move_state() const {
    return m_move_state;
}

bool Player::ray_cast(const glm::vec3& start, const glm::vec3& front, glm::ivec3& block_pos, glm::vec3& normal, float distance) {
    glm::vec3 dir = glm::normalize(front);
    //float step = 0.1f;
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
        if (m_world.is_block(glm::ivec3(ix, iy, iz))) {
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

void Player::change_mode(GameMode mode) {
    m_game_mode = mode;
    Logger::info("Change GameMode to {}", to_str(mode));
    if (mode == CREATIVE) {
        is_fly = false;
        m_gait = Gait::WALK;
    } else if (mode == SPECTATOR) {
        is_fly = true;
        m_gait = Gait::RUN;
    }
}

void Player::set_player_pos(const glm::vec3& pos) {
    m_player_pos = pos;
}

void Player::update(float delta_time) {

    update_move(delta_time);
    update_lookup_block();
    check_player_chunk_transition();

    DebugCollector::get().report("player_pos",
        std::format("x: {:.2f} y: {:.2f} z: {:.2f}",
            m_player_pos.x, m_player_pos.y, m_player_pos.z
        ));
    
    DebugCollector::get().report("speed", std::format("Speed: {:.2} m/s", speed));
}

void Player::update_player_move_state(int key, int action) {
    switch(key) {
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
                        y_speed = 0.0f;
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

void Player::update_front_vec(float offset_x, float offset_y) {
    m_yaw += offset_x * m_sensitivity;
    m_pitch += offset_y * m_sensitivity;
    
    m_yaw = std::fmod(m_yaw, 360.0);
    
    if (m_pitch > 89.0f)  m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    
    m_front.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front.y = sin(glm::radians(m_pitch));
    m_front.z = -cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    
    m_front = glm::normalize(m_front);
}

void Player::check_player_chunk_transition() {
    ChunkPos cur_pos = m_world.chunk_pos(m_player_pos.x, m_player_pos.z);
    if (cur_pos != m_player_chunk_pos) {
        m_world.need_gen();
        m_player_chunk_pos = cur_pos;
        auto chunk = m_world.get_chunk(cur_pos);
        if (chunk == nullptr) {
            DebugCollector::get().report("biome", "Biome: Unknown");
        } else {
            DebugCollector::get()
                .report("biome", "Biome: " + get_biome_str(chunk->get_biome()));
        }
        
    }
}

void Player::update_direction() {
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

void Player::update_lookup_block() {
    // calculate the block that is looked 
    glm::ivec3 block_pos;
    glm::vec3 block_normal;
    if(ray_cast(glm::vec3(m_player_pos.x, (m_player_pos.y + 1.6f), m_player_pos.z), m_front, block_pos, block_normal)) {
        m_look_block = LookBlock{block_pos, glm::floor(block_normal)};
    } else {
        m_look_block = std::nullopt;
    }

    if (m_look_block != std::nullopt) {
        if (Input::get_input_state().mouse_state.left) {
            if (m_world.is_block(m_look_block->pos)) {
                m_world.set_block(m_look_block->pos, 0);
                
            }
            Input::get_input_state().mouse_state.left = false;
        }
        if (Input::get_input_state().mouse_state.right) {
            glm::ivec3 near_pos = m_look_block->pos + m_look_block->normal;
            if (!m_world.is_block(near_pos)) {
                auto x= near_pos.x;
                auto y = near_pos.y;
                auto z = near_pos.z;
                AABB block_box = {
                        glm::vec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
                        glm::vec3{static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1)}
                    };
                AABB player_box = get_aabb();
                if (!player_box.intersects(block_box)) {
                    m_world.set_block(near_pos, 1);
                }
                
            }
            Input::get_input_state().mouse_state.right = false;
        }
    }
}

void Player::update_move(float delta_time) {
    // if frame rate less than 1 frame per second, don't update
    if (delta_time > 1.0f) {
        return;
    }
    if (m_game_mode != SPECTATOR) {
        if (m_gait == Gait::RUN) {
            max_speed = RUN_SPEED;
        }
        if (m_gait == Gait::WALK) {
            max_speed = WALK_SPEED;
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
    if (m_move_state.forward || m_move_state.back || m_move_state.left || m_move_state.right || m_move_state.up) {
        direction = glm::vec3(0.0f, 0.0f, 0.0f);
        speed += ACCELERATION * delta_time;
        if (speed > max_speed) {
            speed = max_speed;
        }
    } else {
        speed += -DECELERATION * delta_time;
        if (speed < 0) {
            speed = 0;
            direction = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }

    update_direction();

    move_distance = {direction.x * speed * delta_time, 0.0f, direction.z * speed * delta_time};
    
    if (is_fly) {
        if (m_move_state.up) {
        y_speed = 7.5f;
        }

        if (m_move_state.down) {
            y_speed = -7.5f;
        }

        if (!m_move_state.down && !m_move_state.up) {
            y_speed = 0.0f;
        }
    } else {
        if (m_move_state.up && can_up) {
        y_speed = 7.5;
        can_up = false;
        
        }

        y_speed += -G * delta_time;
    }
    
    move_distance.y = y_speed * delta_time;
    // y
    update_y_move();
    // x
    update_x_move();

    update_z_move();

    if (m_player_pos.y < -15.0f) {
        Logger::warn("y is tow low");
        m_player_pos += glm::vec3(1.0f, 100.0f, 1.0f);
    }

}

void Player::update_x_move() {
    m_player_pos.x += move_distance.x;
    if (m_game_mode == SPECTATOR) {
        return;
    }
    AABB player_box = get_aabb();
    int minx = std::floor(player_box.min.x);
    int maxx = std::floor(player_box.max.x);
    int miny = std::floor(player_box.min.y);
    int maxy = std::floor(player_box.max.y);
    int minz = std::floor(player_box.min.z);
    int maxz = std::floor(player_box.max.z);

    for (int x = minx; x <= maxx; ++x) {
        for (int y = miny; y <= maxy; ++y) {
            for (int z = minz; z <= maxz; ++z) {
                if (m_world.is_block(glm::vec3{x, y, z})) {
                    AABB block_box = {
                        glm::vec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
                        glm::vec3{static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1)}
                    };
                    if (player_box.intersects(block_box)) {
                        m_gait = Gait::WALK;
                        m_player_pos.x -= move_distance.x;
                        return;
                    }
                }
            }
        }
    }
}

void Player::update_y_move() {
    m_player_pos.y += move_distance.y;
    if (m_game_mode == SPECTATOR) {
        return;
    }
    AABB player_box = get_aabb();
    int minx = std::floor(player_box.min.x);
    int maxx = std::floor(player_box.max.x);
    int miny = std::floor(player_box.min.y);
    int maxy = std::floor(player_box.max.y);
    int minz = std::floor(player_box.min.z);
    int maxz = std::floor(player_box.max.z);

    for (int x = minx; x <= maxx; ++x) {
        for (int y = miny; y <= maxy; ++y) {
            for (int z = minz; z <= maxz; ++z) {
                if (m_world.is_block(glm::vec3{x, y, z})) {
                    AABB block_box = {
                        glm::vec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
                        glm::vec3{static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1)}
                    };
                    if (player_box.intersects(block_box)) {
                        m_player_pos.y -= move_distance.y;
                        y_speed = 0.0f;
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

void Player::update_z_move() {
    m_player_pos.z += move_distance.z;
    if (m_game_mode == SPECTATOR) {
        return;
    }
    AABB player_box = get_aabb();
    int minx = std::floor(player_box.min.x);
    int maxx = std::floor(player_box.max.x);
    int miny = std::floor(player_box.min.y);
    int maxy = std::floor(player_box.max.y);
    int minz = std::floor(player_box.min.z);
    int maxz = std::floor(player_box.max.z);

    for (int x = minx; x <= maxx; ++x) {
        for (int y = miny; y <= maxy; ++y) {
            for (int z = minz; z <= maxz; ++z) {
                if (m_world.is_block(glm::vec3{x, y, z})) {
                    AABB block_box = {
                        glm::vec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
                        glm::vec3{static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1)}
                    };
                    if (player_box.intersects(block_box)) {
                        m_gait = Gait::WALK;
                        m_player_pos.z -= move_distance.z;
                        return;
                    }
                }
            }
        }
    }
}

void Player::update_scroll(double yoffset) {
    if (m_game_mode == SPECTATOR) {
        if (yoffset > 0) {
            max_speed += 1.0f;
        } else {
            if (max_speed > WALK_SPEED) {
                max_speed -= 1.0f;
            }
        }
    }   
}


}
