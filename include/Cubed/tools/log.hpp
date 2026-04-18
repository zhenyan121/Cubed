#pragma once
#include <syncstream>
#include <iostream>
#include <chrono>
#include <format>
#include <source_location>
#include <string>

namespace Logger {
    enum class Level {
        TRACE,
        DEBUG,
        INFO,
        ERROR,
        WARN
    };
    
    template<typename... Args>
    inline void info(std::format_string<Args...> fmt, Args&&... args) {
        auto now_time = std::chrono::
                        time_point_cast<std::chrono::seconds> 
                        (std::chrono::system_clock::now());
        
        std::string msg = std::vformat(fmt.get(), std::make_format_args(args...));
        std::osyncstream(std::cout) << "\033[1;32m" 
                  << std::format("[INFO][{:%Y-%m-%d %H:%M:%S}]", now_time) 
                  << msg 
                  << "\033[0m" 
                  << "\n";
    }

    template<typename... Args>
    inline void error(std::format_string<Args...> fmt, Args&&... args) {
        auto now_time = std::chrono::
                        time_point_cast<std::chrono::seconds> 
                        (std::chrono::system_clock::now());
        std::string msg = std::vformat(fmt.get(), std::make_format_args(args...));
        std::osyncstream(std::cerr) << "\033[1;31m" 
                  << std::format("[ERROR][{:%Y-%m-%d %H:%M:%S}]", now_time) 
                  << msg 
                  << "\033[0m" 
                  << "\n";
    
    }

    template<typename... Args>
    inline void warn(std::format_string<Args...> fmt, Args&&... args) {
        auto now_time = std::chrono::
                        time_point_cast<std::chrono::seconds> 
                        (std::chrono::system_clock::now());
        std::string msg = std::vformat(fmt.get(), std::make_format_args(args...));
        std::osyncstream(std::cout) << "\033[1;33m" 
                  << std::format("[WARN][{:%Y-%m-%d %H:%M:%S}]", now_time) 
                  << msg 
                  << "\033[0m" 
                  << "\n";
    }

    template<typename... Args>
    inline void log(Level level, std::source_location loc, std::format_string<Args...> fmt, Args&&... args) {
        auto now_time = std::chrono::
                        time_point_cast<std::chrono::seconds>
                        (std::chrono::system_clock::now());
        std::string msg = std::vformat(fmt.get(), std::make_format_args(args...));
        switch (level) {
            case Logger::Level::TRACE:
                std::osyncstream(std::cout) << "\033[1;34m"
                  << std::format("[TRACE][{:%Y-%m-%d %H:%M:%S}]", now_time)
                  << "[" << loc.file_name() << ":" << loc.line() << "]"
                  << "[" << loc.function_name() << "]"
                  << msg
                  << "\033[0m"
                  << "\n";
                break;
            case Logger::Level::DEBUG:
                std::osyncstream(std::cout) << "\033[1;34m"
                  << std::format("[DEBUG][{:%Y-%m-%d %H:%M:%S}]", now_time)
                  << msg
                  << "\033[0m"
                  << "\n";
                break;
            case Logger::Level::INFO:
                info(fmt, std::forward<Args>(args)...);
                break;
            case Logger::Level::WARN:
                warn(fmt, std::forward<Args>(args)...);
                break;
            case Logger::Level::ERROR:
                error(fmt, std::forward<Args>(args)...);
                break;
            
        }
        
    } 

} 
