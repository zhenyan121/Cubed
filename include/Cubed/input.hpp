#pragma once

#include <GLFW/glfw3.h>

struct MoveState {
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool down = false;
    bool up = false;
};

struct MouseState {
    bool left = false;
    bool right = false;
};

struct KeyState {
    bool r = false;
};

struct InputState {
    MoveState move_state;
    MouseState mouse_state;
    KeyState key_state;
};

namespace Input {
    InputState& get_input_state();

    
}