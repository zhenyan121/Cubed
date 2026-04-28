#pragma once
#include "Cubed/tools/cubed_assert.hpp"

#include <glm/glm.hpp>

namespace Cubed {

enum class Color {
    BLACK,
    WHITE,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    CYAN,
    MAGENTA,
    GRAY,
    ORANGE,
    PURPLE,
    PINK,
    BROWN
};

inline constexpr glm::vec4 color_value(Color color) {
    using glm::vec4;

    switch (color) {
    case Color::BLACK:
        return vec4{0.0f, 0.0f, 0.0f, 1.0f};
    case Color::WHITE:
        return vec4{1.0f, 1.0f, 1.0f, 1.0f};
    case Color::RED:
        return vec4{1.0f, 0.0f, 0.0f, 1.0f};
    case Color::GREEN:
        return vec4{0.0f, 1.0f, 0.0f, 1.0f};
    case Color::BLUE:
        return vec4{0.0f, 0.0f, 1.0f, 1.0f};
    case Color::YELLOW:
        return vec4{1.0f, 1.0f, 0.0f, 1.0f};
    case Color::CYAN:
        return vec4{0.0f, 1.0f, 1.0f, 1.0f};
    case Color::MAGENTA:
        return vec4{1.0f, 0.0f, 1.0f, 1.0f};
    case Color::GRAY:
        return vec4{0.5f, 0.5f, 0.5f, 1.0f};
    case Color::ORANGE:
        return vec4{1.0f, 0.647f, 0.0f, 1.0f};
    case Color::PURPLE:
        return vec4{0.502f, 0.0f, 0.502f, 1.0f};
    case Color::PINK:
        return vec4{1.0f, 0.753f, 0.769f, 1.0f};
    case Color::BROWN:
        return vec4{0.647f, 0.165f, 0.165f, 1.0f};
    default:
        ASSERT_MSG(false, "Unknown Color");
        return vec4{1.0f, 1.0f, 1.0f, 1.0f};
    }
}

inline glm::vec4 rgb255_to_float(int r, int g, int b, int a) {
    float nr = static_cast<float>(r) / 255.0f;
    float ng = static_cast<float>(g) / 255.0f;
    float nb = static_cast<float>(b) / 255.0f;
    float na = static_cast<float>(a) / 255.0f;

    return glm::vec4{nr, ng, nb, na};
}

} // namespace Cubed