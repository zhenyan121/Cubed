#pragma once
#include <glm/glm.hpp>

#include <Cubed/config.hpp>
#include <Cubed/gameplay/block.hpp>
#include <Cubed/input.hpp>

#include <optional>
#include <string>

class World;

class Player {
private:
    constexpr static float ACCELERATION = 30.0f;   
    constexpr static float DECELERATION = 50.0f; 
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    
    float m_sensitivity = 0.05f;

    float m_speed = 5.0f;
    
    // player is tow block tall, the pos is the lower pos
    glm::vec3 m_player_pos = glm::vec3(0.0f, 15.0f, 0.0f);
    glm::vec3 m_front = glm::vec3(0, 0, -1);
    glm::vec3 m_right;
    MoveState m_move_state;

    std::optional<LookBlock> m_look_block = std::nullopt;
    std::string m_name;
    World& m_world;

    bool ray_cast(const glm::vec3& start, const glm::vec3& dir, glm::ivec3& block_pos, glm::vec3& normal, float distance = 4.0f);

public:
    Player(World& world, const std::string& name);
    ~Player();
    const glm::vec3& get_front() const;
    const std::optional<LookBlock>& get_look_block_pos() const;
    const glm::vec3& get_player_pos() const;
    const MoveState& get_move_state() const;
    
    void set_player_pos(const glm::vec3& pos);
    void update(float delta_time);
    void update_front_vec(float offset_x, float offset_y);
    void update_player_move_state(int key, int action);


};