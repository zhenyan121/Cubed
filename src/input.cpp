#include <Cubed/input.hpp>
#include <Cubed/tools/cubed_assert.hpp>
namespace Cubed {


static InputState input_state;

namespace Input {

    InputState& get_input_state() {
        return input_state;
    }
  
}


}