#include "Cubed/camera.hpp"

static glm::vec3 cameraPos;
static float speed = 0.1f;
static float lastMouseX, lastMouseY;
static bool firseMouse = true;


static MoveState moveState;

static float yaw, pitch;
static glm::vec3 front = glm::vec3(0, 0, -1);
static float sensitivity = 0.05f;
static glm::vec3 rightPos;

void updateMoveCamera() {
    rightPos = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (moveState.forward) {
        cameraPos += glm::vec3(front.x, 0.0f, front.z) * speed;
    }
    if (moveState.back) {
        cameraPos -= glm::vec3(front.x, 0.0f, front.z) * speed;
    }
    if (moveState.left) {
        cameraPos -= glm::vec3(rightPos.x, 0.0f, rightPos.z) * speed;
    }
    if (moveState.right) {
        cameraPos += glm::vec3(rightPos.x, 0.0f, rightPos.z) * speed;
    }
    if (moveState.up) {
        cameraPos += glm::vec3(0.0f, 1.0f, 0.0f) * speed;;
    }
    if (moveState.down) {
        cameraPos -= glm::vec3(0.0f, 1.0f, 0.0f) * speed;;
    }
}


void cameraInit() {
    cameraPos = glm::vec3(0.0f, 2.0f, 0.0f);
}

void changeView(float offsetX, float offsetY) {
    yaw += offsetX * sensitivity;
    pitch += offsetY * sensitivity;

    
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    
    front.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    
    front = glm::normalize(front);
}

void updateCursorPositionCamera(double xpos, double ypos) {
    if (firseMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firseMouse = false;
    }

    float offsetX = xpos - lastMouseX;
    float offsetY = lastMouseY - ypos;

    lastMouseX = xpos;
    lastMouseY = ypos;
    changeView(offsetX, offsetY);
}

glm::mat4 getCameraLookAt() {
    return glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
}

void updateCameraKey(int key, int action) {
    switch(key) {
        case GLFW_KEY_W:
            if (action == GLFW_PRESS) {
                moveState.forward = true;
            }
            if (action == GLFW_RELEASE) {
                moveState.forward = false;
            }
            break;
        case GLFW_KEY_S:
            if (action == GLFW_PRESS) {
                moveState.back = true;
            }
            if (action == GLFW_RELEASE) {
                moveState.back = false;
            }
            break;
        case GLFW_KEY_A:
            if (action == GLFW_PRESS) {
                moveState.left = true;
            }
            if (action == GLFW_RELEASE) {
                moveState.left = false;
            }
            break;
        case GLFW_KEY_D:
            if (action == GLFW_PRESS) {
                moveState.right = true;
            }
            if (action == GLFW_RELEASE) {
                moveState.right = false;
            }
            break;
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS) {
                moveState.up = true;
            }
            if (action == GLFW_RELEASE) {
                moveState.up = false;
            }
            break;
        case GLFW_KEY_LEFT_SHIFT:
            if (action == GLFW_PRESS) {
                moveState.down = true;
            }
            if (action == GLFW_RELEASE) {
                moveState.down = false;
            }
            break;
        
            
    }
}
