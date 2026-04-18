#pragma once
#include <glm/glm.hpp>

#include <Cubed/AABB.hpp>
#include <Cubed/config.hpp>
#include <Cubed/gameplay/block.hpp>
#include <Cubed/gameplay/chunk_pos.hpp>
#include <Cubed/gameplay/game_mode.hpp>
#include <Cubed/input.hpp>

#include <optional>
#include <string>

enum class Gait{
    WALK,
    RUN
};

class World;

class Player {
private:
    using enum GameMode;
    constexpr static float WALK_SPEED = 4.5f;
    constexpr static float RUN_SPEED = 7.0f;
    constexpr static float ACCELERATION = 10.0f;   
    constexpr static float DECELERATION = 15.0f;
    constexpr static float G = 22.5f;

    constexpr static float MAX_SPACE_ON_TIME = 0.3f; 
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    
    float m_sensitivity = 0.15f;

    float max_speed = WALK_SPEED;
    float y_speed = 0.0f;
    bool can_up = true;
    
    float space_on_time = 0.0f;
    bool space_on = false;
    bool is_fly = false;
    
    float speed = 0;
    
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 move_distance {0.0f, 0.0f, 0.0f};
    // player is tow block tall, the pos is the lower pos
    
    glm::vec3 m_player_pos {0.0f, 120.0f, 0.0f};
    ChunkPos m_player_chunk_pos {0, 0};

    glm::vec3 m_front {0, 0, -1};
    glm::vec3 m_right {0, 0, 0};
    glm::vec3 m_size {0.6f, 1.8f, 0.6f};

    Gait m_gait = Gait::WALK;
    MoveState m_move_state {};
    GameMode m_game_mode = CREATIVE;
    std::optional<LookBlock> m_look_block = std::nullopt;
    std::string m_name {};
    World& m_world;

    bool ray_cast(const glm::vec3& start, const glm::vec3& dir, glm::ivec3& block_pos, glm::vec3& normal, float distance = 4.0f);

    void check_player_chunk_transition();
    void update_direction();
    void update_lookup_block();
    void update_move(float delta_time);
    void update_x_move();
    void update_y_move();
    void update_z_move();

public:
    Player(World& world, const std::string& name);
    ~Player();
    AABB get_aabb() const;
    const glm::vec3& get_front() const;
    const Gait& get_gait() const;
    const std::optional<LookBlock>& get_look_block_pos() const;
    const glm::vec3& get_player_pos() const;
    const MoveState& get_move_state() const;
    
    void change_mode(GameMode mode);
    void set_player_pos(const glm::vec3& pos);
    void update(float delta_time);
    void update_front_vec(float offset_x, float offset_y);
    void update_player_move_state(int key, int action);
    void update_scroll(double yoffset);

};