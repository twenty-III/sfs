#pragma once

#include <mutex>
#include <string>
#include <iostream>

class Logger
{
public:
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    static Logger &instance() {
        static Logger logger;
        return logger;
    }

    template <typename... Args>
    void log(const std::string &type, Args &&...args);

private:
    std::mutex log_mutex_;

    Logger() = default;
};

template <typename... Args>
void Logger::log(const std::string &type, Args &&...args)
{
    std::lock_guard<std::mutex> lock(log_mutex_);

    auto& stream = (type == "ERROR") ? std::cerr : std::cout;

    stream << "[" << type << "] ";
    (stream << ... << std::forward<Args>(args));
    stream << '\n';
}

#define LOG_INFO(...)  Logger::instance().log("INFO", __VA_ARGS__)
#define LOG_ERROR(...) Logger::instance().log("ERROR", __VA_ARGS__)
#define LOG_WARN(...)  Logger::instance().log("WARN", __VA_ARGS__)