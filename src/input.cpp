#include "Cubed/input.hpp"

namespace Cubed {

static InputState input_state;

namespace Input {

InputState& get_input_state() { return input_state; }

} // namespace Input

} // namespace Cubed