#pragma once

#include "Cubed/tools/log.hpp"

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
#elif defined(__linux__)
#include <fstream>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace Cubed {

namespace Tools {

inline bool get_os_version(std::string& str) {
#ifdef _WIN32
    HMODULE hntdll = GetModuleHandleW(L"ntdll.dll");
    if (!hntdll)
        return false;

    auto prtl_get_version = reinterpret_cast<RtlGetVersionPtr>(
        GetProcAddress(hntdll, "RtlGetVersion"));
    if (!prtl_get_version)
        return false;

    RTL_OSVERSIONINFOW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (prtl_get_version(&osvi) != 0)
        return false;
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
#elif defined(__linux__)
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
#else
    str = "Unknown OS";
    return false;
#endif
}

inline size_t get_current_rss() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#elif defined(__linux__)
    std::ifstream statm("/proc/self/statm");
    long vsz = 0, rss_pages = 0;
    statm >> vsz >> rss_pages;
    statm.close();
    long page_size = sysconf(_SC_PAGESIZE);
    return rss_pages * page_size;
#else
    return 0; // Unsupported platform
#endif
}

inline std::string get_cpu_info() {
#ifdef _WIN32
    HKEY h_key;
    std::string cpu_name;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
                      KEY_READ, &h_key) == ERROR_SUCCESS) {

        DWORD dw_size = 0;
        if (RegQueryValueExW(h_key, L"ProcessorNameString", NULL, NULL, NULL,
                             &dw_size) == ERROR_SUCCESS &&
            dw_size > 0) {
            std::vector<wchar_t> buffer(dw_size / sizeof(wchar_t));
            if (RegQueryValueExW(h_key, L"ProcessorNameString", NULL, NULL,
                                 (LPBYTE)buffer.data(),
                                 &dw_size) == ERROR_SUCCESS) {
                int len = WideCharToMultiByte(CP_UTF8, 0, buffer.data(), -1,
                                              NULL, 0, NULL, NULL);
                if (len > 0) {
                    std::vector<char> narrow(len);
                    WideCharToMultiByte(CP_UTF8, 0, buffer.data(), -1,
                                        narrow.data(), len, NULL, NULL);
                    cpu_name = narrow.data();
                }
            }
        }
        RegCloseKey(h_key);
    }
    if (cpu_name.empty()) {
        cpu_name = "Unknown";
    }
    return cpu_name;
#elif defined(__linux__)
    std::ifstream file("/proc/cpuinfo");
    if (!file.is_open()) {
        return std::string{"Unkown"};
    }
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(":");
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, pos - 1);
        key = key.erase(key.find_last_not_of(" \t\n") + 1);
        if (key != "model name") {
            continue;
        }
        return line.substr(pos + 2);
    }
    return std::string{"Unkown"};
#else
    return std::string{"Unkown"};
#endif
}

} // namespace Tools

} // namespace Cubed
