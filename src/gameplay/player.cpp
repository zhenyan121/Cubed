#include <Cubed/gameplay/player.hpp>
#include <GLFW/glfw3.h>
Player::Player() {

}

const glm::vec3& Player::getFront() const {
    return m_front;
}

const glm::vec3& Player::getPlayerPos() const {
    return m_playerPos;
}

const MoveState& Player::getMoveState() const {
    return m_moveState;
}

void Player::setPlayerPos(const glm::vec3& pos) {
    m_playerPos = pos;
}

void Player::update(float deltaTime) {
    m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (m_moveState.forward) {
        m_playerPos += glm::vec3(m_front.x, 0.0f, m_front.z) * m_speed;
    }
    if (m_moveState.back) {
        m_playerPos -= glm::vec3(m_front.x, 0.0f, m_front.z) * m_speed;
    }
    if (m_moveState.left) {
        m_playerPos -= glm::vec3(m_right.x, 0.0f, m_right.z) * m_speed;
    }
    if (m_moveState.right) {
        m_playerPos += glm::vec3(m_right.x, 0.0f, m_right.z) * m_speed;
    }
    if (m_moveState.up) {
        m_playerPos += glm::vec3(0.0f, 1.0f, 0.0f) * m_speed;;
    }
    if (m_moveState.down) {
        m_playerPos -= glm::vec3(0.0f, 1.0f, 0.0f) * m_speed;;
    }
}

void Player::updatePlayerMoveState(int key, int action) {
    switch(key) {
        case GLFW_KEY_W:
            if (action == GLFW_PRESS) {
                m_moveState.forward = true;
            }
            if (action == GLFW_RELEASE) {
                m_moveState.forward = false;
            }
            break;
        case GLFW_KEY_S:
            if (action == GLFW_PRESS) {
                m_moveState.back = true;
            }
            if (action == GLFW_RELEASE) {
                m_moveState.back = false;
            }
            break;
        case GLFW_KEY_A:
            if (action == GLFW_PRESS) {
                m_moveState.left = true;
            }
            if (action == GLFW_RELEASE) {
                m_moveState.left = false;
            }
            break;
        case GLFW_KEY_D:
            if (action == GLFW_PRESS) {
                m_moveState.right = true;
            }
            if (action == GLFW_RELEASE) {
                m_moveState.right = false;
            }
            break;
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS) {
                m_moveState.up = true;
            }
            if (action == GLFW_RELEASE) {
                m_moveState.up = false;
            }
            break;
        case GLFW_KEY_LEFT_SHIFT:
            if (action == GLFW_PRESS) {
                m_moveState.down = true;
            }
            if (action == GLFW_RELEASE) {
                m_moveState.down = false;
            }
            break;
        
            
    }
}

void Player::updateFrontVec(float offsetX, float offsetY) {
    m_yaw += offsetX * m_sensitivity;
    m_pitch += offsetY * m_sensitivity;
    
    
    if (m_pitch > 89.0f)  m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    
    m_front.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front.y = sin(glm::radians(m_pitch));
    m_front.z = -cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    
    m_front = glm::normalize(m_front);
}