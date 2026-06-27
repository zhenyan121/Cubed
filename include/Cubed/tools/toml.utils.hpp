#pragma once

#include "Cubed/tools/log.hpp"

#include <toml++/toml.hpp>
namespace Cubed {
namespace TOML {

template <typename T>
concept TomlValueType =
    std::same_as<std::decay_t<T>, int> || std::same_as<std::decay_t<T>, bool> ||
    std::same_as<std::decay_t<T>, double> ||
    std::same_as<std::decay_t<T>, char> ||
    std::same_as<std::decay_t<T>, toml::date> ||
    std::same_as<std::decay_t<T>, toml::time> ||
    std::same_as<std::decay_t<T>, toml::date_time> ||
    std::same_as<std::decay_t<T>, std::string>;

template <TomlValueType T>
std::optional<T> safe_get_value(const toml::table& table, std::string_view key,
                                const T& default_value) {
    auto value = table[key].value<T>();
    if (value == std::nullopt) {
        Logger::warn("Key {} Is Not Find, Wiil Set the Default Value {}", key,
                     default_value);
        value = default_value;
    }
    return value;
}
template <typename U>
    requires std::convertible_to<U, std::string>
std::optional<std::string> safe_get_value(const toml::table& table,
                                          std::string_view key,
                                          U&& default_value) {
    return safe_get_value<std::string>(
        table, key, std::string(std::forward<U>(default_value)));
}

} // namespace TOML

} // namespace Cubed
