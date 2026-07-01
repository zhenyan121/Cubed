#pragma once
#include "Cubed/AABB.hpp"
#include "Cubed/constants.hpp"
#include "Cubed/gameplay/block.hpp"
#include "Cubed/gameplay/chunk_pos.hpp"
#include "Cubed/gameplay/game_mode.hpp"
#include "Cubed/input.hpp"

#include <absl/container/flat_hash_set.h>
#include <glm/glm.hpp>
#include <optional>
#include <shared_mutex>
namespace Cubed {
enum class Gait { WALK = 0, RUN };
class ClientWorld;
class ClientPlayer {
public:
    using ChunkPosSet = absl::flat_hash_set<ChunkPos, ChunkPos::Hash>;
    ClientPlayer(ClientWorld& world);
    ~ClientPlayer();

    void update_chunk_set(const ChunkPosSet& set);
    const ChunkPosSet& get_chunk_pos_set() const;
    ChunkPosSet& get_chunk_pos_set();

    static AABB get_aabb(const glm::vec3& pos);
    const glm::vec3& get_front() const;
    const Gait& get_gait() const;
    const std::optional<LookBlock>& get_look_block_pos() const;
    // thread safe
    glm::vec3 get_player_pos() const;
    const MoveState& get_move_state() const;

    void change_mode(GameMode mode);
    void hot_reload();
    void set_player_pos(const glm::vec3& pos);
    void set_place_block(unsigned id);
    void update(float delta_time);
    void update_front_vec(float offset_x, float offset_y);
    void update_player_move_state(int key, int action);
    void update_scroll(double yoffset);

    float& max_walk_speed();
    float& max_run_speed();
    float& max_speed();
    float& acceleration();
    float& deceleration();
    float& g();
    float& fly_y_speed();

    unsigned place_block() const;

    Gait& gait();
    GameMode& game_mode();

    const ClientWorld& get_world() const;

    void set_uuid(std::string_view uuid);
    const std::string& get_uuid() const;
    const std::string& get_name() const;

    void init(std::string_view name);

private:
    using enum GameMode;
    float m_max_walk_speed = DEFAULT_MAX_WALK_SPEED;
    float m_max_run_speed = DEFAULT_MAX_RUN_SPEED;
    float m_acceleration = DEFAULT_ACCELERATION;
    float m_deceleration = DEFAULT_DECELERATION;
    float m_g = DEFAULT_G;
    constexpr static float MAX_SPACE_ON_TIME = 0.3f;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    float m_sensitivity = 0.15f;

    float m_max_speed = m_max_walk_speed;
    float m_y_speed = 0.0f;
    float m_fly_y_speed = 7.5f;
    bool can_up = true;

    float space_on_time = 0.0f;
    bool space_on = false;
    bool is_fly = false;

    float m_xz_speed = 0.0f;

    unsigned m_place_block = 1;

    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 move_distance{0.0f, 0.0f, 0.0f};
    // player is tow block tall, the pos is the lower pos

    glm::vec3 m_player_pos{0.0f, 255.0f, 0.0f};
    ChunkPos m_last_chunk_pos{0, 0};

    glm::vec3 m_front{0, 0, -1};
    glm::vec3 m_right{0, 0, 0};
    static constexpr glm::vec3 M_SIZE{0.6f, 1.8f, 0.6f};

    Gait m_gait = Gait::WALK;
    MoveState m_move_state{};
    GameMode m_game_mode = CREATIVE;
    std::optional<LookBlock> m_look_block = std::nullopt;
    std::string m_name{};
    std::string m_uuid;
    ClientWorld& m_world;

    mutable std::shared_mutex m_player_pos_mutex;
    mutable std::shared_mutex m_chunk_pos_mutex;
    ChunkPosSet m_player_chunk_pos_set;
    bool ray_cast(const glm::vec3& start, const glm::vec3& dir,
                  glm::ivec3& block_pos, glm::vec3& normal,
                  float distance = 4.0f);

    void update_direction();
    void update_lookup_block();
    void update_move(float delta_time);
    void update_x_move(glm::vec3& player_pos);
    void update_y_move(glm::vec3& player_pos);
    void update_z_move(glm::vec3& player_pos);
    void update_player_chunk();
};
} // namespace Cubed
