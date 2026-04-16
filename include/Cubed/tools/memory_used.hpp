#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#include <fstream>
#endif

inline size_t get_current_rss() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
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