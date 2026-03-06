#include <memory>
#include <optional>
#include <Cubed/camera.hpp>
#include <Cubed/gameplay/player.hpp>
#include <Cubed/tools/cubed_assert.hpp>

Camera::Camera() {

}

void Camera::updateMoveCamera() {
    CUBED_ASSERT_MSG(m_player, "nullptr");
    m_cameraPos = m_player->getPlayerPos();
}


void Camera::cameraInit(Player* _player) {
    m_cameraPos = glm::vec3(0.0f, 2.0f, 0.0f);
    m_player = _player;
}

void Camera::updateCursorPositionCamera(double xpos, double ypos) {
    if (m_firseMouse) {
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firseMouse = false;
    }

    float offsetX = xpos - m_lastMouseX;
    float offsetY = m_lastMouseY - ypos;

    m_lastMouseX = xpos;
    m_lastMouseY = ypos;
    CUBED_ASSERT_MSG(m_player, "nullptr");
    m_player->updateFrontVec(offsetX, offsetY);
}

const glm::mat4 Camera::getCameraLookAt() const{
    CUBED_ASSERT_MSG(m_player, "nullptr");
    return glm::lookAt(m_cameraPos, m_cameraPos + m_player->getFront(), glm::vec3(0.0f, 1.0f, 0.0f));
}
