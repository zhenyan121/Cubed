#include <GLFW/glfw3.h>

#include <Cubed/gameplay/player.hpp>

Player::Player(const World& world, const std::string& name) :
    m_world(world),
    m_name(name)    
{

}

const glm::vec3& Player::get_front() const {
    return m_front;
}

const glm::vec3& Player::get_player_pos() const {
    return m_player_pos;
}

const MoveState& Player::get_move_state() const {
    return m_move_state;
}

void Player::set_player_pos(const glm::vec3& pos) {
    m_player_pos = pos;
}

void Player::update(float delta_time) {

    m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));
    float speed = m_speed * delta_time;
    if (m_move_state.forward) {
        m_player_pos += glm::vec3(m_front.x, 0.0f, m_front.z) * speed;
    }
    if (m_move_state.back) {
        m_player_pos -= glm::vec3(m_front.x, 0.0f, m_front.z) * speed;
    }
    if (m_move_state.left) {
        m_player_pos -= glm::vec3(m_right.x, 0.0f, m_right.z) * speed;
    }
    if (m_move_state.right) {
        m_player_pos += glm::vec3(m_right.x, 0.0f, m_right.z) * speed;
    }
    if (m_move_state.up) {
        m_player_pos += glm::vec3(0.0f, 1.0f, 0.0f) * speed;
    }
    if (m_move_state.down) {
        m_player_pos -= glm::vec3(0.0f, 1.0f, 0.0f) * speed;
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
    
    
    if (m_pitch > 89.0f)  m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    
    m_front.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front.y = sin(glm::radians(m_pitch));
    m_front.z = -cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    
    m_front = glm::normalize(m_front);
}