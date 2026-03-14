#include <Cubed/input.hpp>

static InputState input_state;

namespace Input {
    InputState& get_input_state() {
        return input_state;
    }
}