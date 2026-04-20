#pragma once

#include <stdexcept>
#include <string>

namespace Cubed {


enum class GameMode {
    CREATIVE,
    SPECTATOR
};

inline std::string to_str(GameMode mode) {
    using enum GameMode;
    switch (mode) {
        case CREATIVE:
            return {"Creative"};
        case SPECTATOR:
            return {"Spective"};
    }
    throw std::invalid_argument{"GameMode is invaild"};
}


}