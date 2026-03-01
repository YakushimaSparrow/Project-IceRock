#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

enum class LoggerLevel{
    INFO, DEBUG, WARNING, ERROR
};

class Logger{
    private:
        static std::unique_ptr<Logger> instance;
        std::shared_ptr<spdlog::logger> logger;
        Logger(const std::string& path = "log.txt");
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
    public:
        void log(const std::string& msg, LoggerLevel ll);
        static Logger& getInstance(const std::string& path = "log.txt");
};