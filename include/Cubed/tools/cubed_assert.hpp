#pragma once
#include <Cubed/tools/log.hpp>
namespace Assert {
    inline void msg(const char* condition, const char* file,
             int line, const char* func,
             const std::string& message = ""        
            ) {
    
        LOG::error("Assertion failed: {} at {}: {} in function {}",
             condition, file, line, func);
        if (!message.empty()) {
            LOG::error("Message: {}", message);
        }
        std::abort();

    }
}

#ifdef NDEBUG
#define CUBED_ASSERT(cond) ((void)0)
#define CUBED_ASSERT_MSG(cond, message) ((void)0)
#else
#define CUBED_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            ::Assert::msg(#cond, __FILE__, __LINE__, __func__); \
        } \
    } while (0)
#define CUBED_ASSERT_MSG(cond, message) \
    do { \
        if (!(cond)) { \
            ::Assert::msg(#cond, __FILE__, __LINE__, __func__, message); \
        } \
    } while (0)
#endif