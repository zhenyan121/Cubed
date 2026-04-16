#pragma once

#include <string>
#include <fstream>

#include <Cubed/tools/log.hpp>

#ifdef _WIN32

#elif defined(__linux__)
inline bool get_os_version(std::string& str) {
    std::ifstream file("/etc/os-release");
    if (!file.is_open()) {
        Logger::error("Can't Open /etc/os-release");
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        size_t eq_pos = line.find("=");
        if (eq_pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, eq_pos);
        if (key != "PRETTY_NAME") {
            continue;
        }
        str = line.substr(eq_pos + 1);
        
        if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
            str = str.substr(1, str.size() - 2);
            return true;
        }
    }
    return false;
}
#endif


