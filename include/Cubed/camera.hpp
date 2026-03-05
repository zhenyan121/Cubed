#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


struct MoveState {
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool down = false;
    bool up = false;
};

void updateMoveCamera();

void cameraInit();

void changeView(float offsetX, float offsetY);

void updateCursorPositionCamera(double xpos, double ypos);

glm::mat4 getCameraLookAt();

void updateCameraKey(int key, int action);
