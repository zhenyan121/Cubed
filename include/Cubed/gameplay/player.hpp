#pragma once
#include <glm/glm.hpp>

#include <Cubed/config.hpp>

#include <optional>
#include <string>
struct MoveState {
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool down = false;
    bool up = false;
};

class World;

class Player {
private:

    float m_yaw;
    float m_pitch;
    
    float m_sensitivity = 0.05f;

    float m_speed = 10.0f;
    // player is tow block tall, the pos is the lower pos
    glm::vec3 m_player_pos = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3 m_front = glm::vec3(0, 0, -1);
    glm::vec3 m_right;
    MoveState m_move_state;

    std::optional<glm::ivec3> m_look_block_pos = std::nullopt;
    std::string m_name;
    World& m_world;

    bool ray_cast(const glm::vec3& start, const glm::vec3& dir, glm::ivec3& block_pos, float distance = 4.0f);

public:
    Player(World& world, const std::string& name);
    ~Player();
    const glm::vec3& get_front() const;
    const std::optional<glm::ivec3>& get_look_block_pos() const;
    const glm::vec3& get_player_pos() const;
    const MoveState& get_move_state() const;
    
    void set_player_pos(const glm::vec3& pos);
    void update(float delta_time);
    void update_front_vec(float offset_x, float offset_y);
    void update_player_move_state(int key, int action);


};