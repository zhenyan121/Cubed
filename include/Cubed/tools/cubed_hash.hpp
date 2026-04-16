#pragma once
#include <string_view>
namespace HASH {
    inline std::size_t str(std::string_view value) {
        return std::hash<std::string_view>{}(value);
    }
    
}