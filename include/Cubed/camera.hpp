#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Player;


class Camera {
private:

    bool m_firseMouse = true;
    Player* m_player;
    float m_lastMouseX, m_lastMouseY;
    glm::vec3 m_cameraPos;
    

public:

    Camera();

    void updateMoveCamera();

    void cameraInit(Player* player);

    void updateCursorPositionCamera(double xpos, double ypos);

    const glm::mat4 getCameraLookAt() const;

};


