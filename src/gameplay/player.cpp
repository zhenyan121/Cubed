#include <Cubed/gameplay/player.hpp>
#include <Cubed/gameplay/world.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/tools/log.hpp>
#include <GLFW/glfw3.h>


Player::Player(World& world, const std::string& name) :
    m_world(world),
    m_name(name)    
{

}
Player::~Player() {

}
const glm::vec3& Player::get_front() const {
    return m_front;
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
    float step = 0.1f;
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


void Player::set_player_pos(const glm::vec3& pos) {
    m_player_pos = pos;
}

void Player::update(float delta_time) {
    static float speed = 0;
    static glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
    if (m_move_state.forward || m_move_state.back || m_move_state.left || m_move_state.right || m_move_state.up) {
        direction = glm::vec3(0.0f, 0.0f, 0.0f);
        speed += ACCELERATION * delta_time;
        if (speed > m_speed) {
            speed = m_speed;
        }
    } else {
        speed += -DECELERATION * delta_time;
        if (speed < 0) {
            speed = 0;
            direction = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }

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

    if (glm::length(direction) > 0.001f) {
        auto new_pos = m_player_pos + direction * speed * delta_time;
        new_pos.y += 1.0f;
        if (m_world.can_move(new_pos)) {
            new_pos.y -= 1.0f;
            m_player_pos = new_pos;
        } else {
            if (glm::length(move_dir_front) > 0.001f) {
                if (std::abs(move_dir_front.x) > std::abs(move_dir_front.z)) {
                    move_dir_front.z = 0.0f;
                } else {
                    move_dir_front.x = 0.0f;
                }
                direction = glm::normalize(move_dir_front);
                auto new_pos = m_player_pos + direction * speed * delta_time;
                new_pos.y += 1.0f;
                if (m_world.can_move(new_pos)) {
                    new_pos.y -= 1.0f;
                    m_player_pos = new_pos;
                } else {
                    direction = glm::vec3(0.0f, 0.0f, 0.0f);
                }
            }
            if (glm::length(move_dir_right) > 0.001f) {
                if (std::abs(move_dir_right.x) > std::abs(move_dir_right.z)) {
                    move_dir_right.z = 0.0f;
                } else {
                    move_dir_right.x = 0.0f;
                }
                direction = glm::normalize(move_dir_right);
                auto new_pos = m_player_pos + direction * speed * delta_time;
                new_pos.y += 1.0f;
                if (m_world.can_move(new_pos)) {
                    new_pos.y -= 1.0f;
                    m_player_pos = new_pos;
                } else {
                    direction = glm::vec3(0.0f, 0.0f, 0.0f);
                }
            }
        }
    }
    

    

    if (m_move_state.up) {
        auto new_pos = m_player_pos + glm::vec3(0.0f, 1.0f, 0.0f) * speed * 2.0f * delta_time;
        new_pos.y += 2.0f;
        if (!m_world.is_block(glm::floor(new_pos))) {
            new_pos.y -= 2.0f;
            m_player_pos = new_pos;
        }
    }

    /*
    if (m_move_state.down) {
        m_player_pos -= glm::vec3(0.0f, 1.0f, 0.0f) * speed;
    }
    */
    // calculate the block that is looked 
    glm::ivec3 block_pos;
    glm::vec3 block_normal;
    if(ray_cast(glm::vec3(m_player_pos.x, (m_player_pos.y + 1.6f), m_player_pos.z), m_front, block_pos, block_normal)) {
        m_look_block = std::move(LookBlock{block_pos, glm::floor(block_normal)});
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
                glm::ivec3 p_pos = glm::floor(m_player_pos);
                if ((near_pos != p_pos) && (near_pos != (p_pos + glm::ivec3(0 ,1, 0))) && (near_pos != (p_pos + glm::ivec3(0, 2, 0)))) {
                    m_world.set_block(near_pos, 1);
                }
                
            }
            Input::get_input_state().mouse_state.right = false;
        }
    }


    static float down_speed = 0.0f;

    if (!m_world.is_block(glm::floor(m_player_pos)) && !m_move_state.up) {
        down_speed += G * delta_time;
        m_player_pos -= glm::vec3(0.0f, 1.0f, 0.0f) * down_speed * delta_time;
    } else {
        down_speed = 0.0f;
    }

    if (m_player_pos.y < -50.0f) {
        m_player_pos = glm::vec3(0.0f, 16.0f, 0.0f);
    }



}

void Player::update_player_move_state(int key, int action) {
    switch(key) {
        case GLFW_KEY_W:
            if (action == GLFW_PRESS) {
                m_move_state.forward = true;
            }
            if (action == GLFW_RELEASE) {
                m_move_state.forward = false;
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