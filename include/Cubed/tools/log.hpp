#pragma once
#include <iostream>
#include <chrono>
#include <format>
#include <string>
namespace LOG {

    template<typename... Args>
    void info(const char * fmt, Args&&... args) {
        auto now_time = std::chrono::
                        time_point_cast<std::chrono::seconds> 
                        (std::chrono::system_clock::now());
        
        std::string msg = std::vformat(fmt, std::make_format_args(args...));
        std::cout << "\033[1;32m" 
                  << std::format("[INFO][{:%Y-%m-%d %H:%M:%S}]", now_time) 
                  << msg 
                  << "\033[0m" 
                  << std::endl;
    }

    template<typename... Args>
    void error(const char * fmt, Args&&... args) {
        auto now_time = std::chrono::
                        time_point_cast<std::chrono::seconds> 
                        (std::chrono::system_clock::now());
        std::string msg = std::vformat(fmt, std::make_format_args(args...));
        std::cout << "\033[1;31m" 
                  << std::format("[ERROR][{:%Y-%m-%d %H:%M:%S}]", now_time) 
                  << msg 
                  << "\033[0m" 
                  << std::endl;
    
    }

    template<typename... Args>
    void warn(const char * fmt, Args&&... args) {
        auto now_time = std::chrono::
                        time_point_cast<std::chrono::seconds> 
                        (std::chrono::system_clock::now());
        std::string msg = std::vformat(fmt, std::make_format_args(args...));
        std::cout << "\033[1;33m" 
                  << std::format("[WARN][{:%Y-%m-%d %H:%M:%S}]", now_time) 
                  << msg 
                  << "\033[0m" 
                  << std::endl;
    }

} 
