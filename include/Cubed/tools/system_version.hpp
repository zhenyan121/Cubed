#pragma once

#include <string>
#include <Cubed/tools/log.hpp>

#ifdef _WIN32
#include <windows.h>
typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
inline bool get_os_version(std::string& str) {
    HMODULE hntdll = GetModuleHandleW(L"ntdll.dll");
    if (!hntdll) return false;

    auto prtl_get_version = reinterpret_cast<RtlGetVersionPtr>(
        GetProcAddress(hntdll, "RtlGetVersion"));
    if (!prtl_get_version) return false;

    RTL_OSVERSIONINFOW osvi = { 0 };
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (prtl_get_version(&osvi) != 0) return false;
    if (osvi.dwMajorVersion == 10) {
        if (osvi.dwBuildNumber >= 22000) {
            str = "Windows 11 Build " + std::to_string(osvi.dwBuildNumber);
        } else {
            str = "Windows 10 Build " + std::to_string(osvi.dwBuildNumber);
        }
    } else {
        str = "Windows Build " + std::to_string(osvi.dwBuildNumber);
    }
    return true;
}

#elif defined(__linux__)
#include <fstream>

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
#else
inline bool get_os_version(std::string& str) {
    str = "Unknown OS";
    return false;
}
#endif


