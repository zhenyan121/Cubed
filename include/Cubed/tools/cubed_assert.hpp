#pragma once
#include "Cubed/tools/log.hpp"

namespace Cubed {

namespace Assert {
inline void msg(const char* condition, const char* file, int line,
                const char* func, std::string_view message = "") {

    Logger::error("Assertion failed: {} at {}: {} in function {}", condition,
                  file, line, func);
    if (message.size()) {
        Logger::error("Message: {}", message);
    }
    std::abort();
}
} // namespace Assert

#ifdef DEBUG_MODE
#define ASSERT(cond)                                                           \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::Cubed::Assert::msg(#cond, __FILE__, __LINE__, __func__);         \
        }                                                                      \
    } while (0)
#define ASSERT_MSG(cond, message)                                              \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::Cubed::Assert::msg(#cond, __FILE__, __LINE__, __func__,          \
                                 message);                                     \
        }                                                                      \
    } while (0)

#else
#define ASSERT(cond) ((void)0)
#define ASSERT_MSG(cond, message) ((void)0)
#endif

} // namespace Cubed