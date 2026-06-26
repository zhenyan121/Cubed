#pragma once

#include <format>
#include <span>
#include <stdexcept>
#include <string_view>

namespace Cubed {
class ArgParser {
public:
    ArgParser(int argc, char** argv) : m_args(argv, argc) {};
    ArgParser(std::span<char*> args) : m_args(args) {}

    bool has_next() const { return m_index < m_args.size(); }

    std::string_view next() {
        if (!has_next()) {
            throw std::runtime_error("No more arguments");
        }
        return m_args[m_index++];
    }

    std::string_view require_next(std::string_view option) {
        if (!has_next()) {
            throw std::runtime_error(
                std::format("{} requires an argument", option));
        }
        return next();
    }

private:
    std::span<char*> m_args;
    size_t m_index = 1;
};
} // namespace Cubed
