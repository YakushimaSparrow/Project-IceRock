#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

enum class LogLevel { info, debug, warn, error };

class Logger {
public:
    static Logger& instance(const std::string& path = "icerock.log");
    void log(const std::string& msg, LogLevel lv = LogLevel::info);
private:
    explicit Logger(const std::string& path);
    std::shared_ptr<spdlog::logger> m_log;
    static std::unique_ptr<Logger> m_inst;
};
