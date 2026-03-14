#pragma once

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

struct InputState {
    MoveState move_state;
    MouseState mouse_state;
};

namespace Input {
    InputState& get_input_state();
}