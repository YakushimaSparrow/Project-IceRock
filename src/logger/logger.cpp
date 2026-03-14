#include "logger.hpp"

std::unique_ptr<Logger> Logger::m_inst = nullptr;

Logger::Logger(const std::string& path)
    : m_log(spdlog::basic_logger_mt("icerock", path, true))
{
    m_log->set_level(spdlog::level::debug);
}

Logger& Logger::instance(const std::string& path)
{
    if (!m_inst) m_inst.reset(new Logger(path));
    return *m_inst;
}

void Logger::log(const std::string& msg, LogLevel lv)
{
    switch (lv) {
        case LogLevel::info:  m_log->info(msg);  break;
        case LogLevel::debug: m_log->debug(msg); break;
        case LogLevel::warn:  m_log->warn(msg);  break;
        case LogLevel::error: m_log->error(msg); break;
    }
}
