#include <Cubed/camera.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/tools/cubed_assert.hpp>

Camera::Camera() {

}

void Camera::update_move_camera() {
    CUBED_ASSERT_MSG(m_player, "nullptr");
    auto pos = m_player->get_player_pos();
    // pos.y need to add 1.6f to center
    m_camera_pos = glm::vec3(pos.x, pos.y + 1.6f, pos.z);
}


void Camera::camera_init(Player* player) {
    m_player = player;
    update_move_camera();
}

void Camera::update_cursor_position_camera(double xpos, double ypos) {
    if (m_firse_mouse) {
        m_last_mouse_x = xpos;
        m_last_mouse_y = ypos;
        m_firse_mouse = false;
    }

    float offset_x = xpos - m_last_mouse_x;
    float offset_y = m_last_mouse_y - ypos;

    m_last_mouse_x = xpos;
    m_last_mouse_y = ypos;
    CUBED_ASSERT_MSG(m_player, "nullptr");
    m_player->update_front_vec(offset_x, offset_y);
}

const glm::mat4 Camera::get_camera_lookat() const{
    CUBED_ASSERT_MSG(m_player, "nullptr");
    return glm::lookAt(m_camera_pos, m_camera_pos + m_player->get_front(), glm::vec3(0.0f, 1.0f, 0.0f));
}
