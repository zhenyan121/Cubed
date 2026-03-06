#pragma once
#include <glm/glm.hpp>

struct MoveState {
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool down = false;
    bool up = false;
};


class Player {
private:

    float m_yaw;
    float m_pitch;
    
    float m_sensitivity = 0.05f;

    float m_speed = 0.1f;
    // player is tow block tall, the pos is the lower pos
    glm::vec3 m_playerPos;
    glm::vec3 m_front = glm::vec3(0, 0, -1);
    glm::vec3 m_right;
    MoveState m_moveState;

public:
    Player();
    
    const glm::vec3& getFront() const;
    const glm::vec3& getPlayerPos() const;
    const MoveState& getMoveState() const;
    void setPlayerPos(const glm::vec3& pos);
    void update(float deltaTime);
    void updateFrontVec(float offsetX, float offsetY);
    void updatePlayerMoveState(int key, int action);


};