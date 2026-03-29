#pragma once
#include <string>
namespace HASH {
    inline std::size_t str(const std::string& value) {
        return std::hash<std::string>{}(value);
    }
    
}