#include "logger.hpp"
#include <iostream>
#include <utility>

std::unique_ptr<Logger> Logger::instance = nullptr;

Logger::Logger(const std::string &path):logger(spdlog::basic_logger_mt("basic_logger", path))
{
    std::cout << "Succesfully created!";
};

void Logger::log(const std::string &msg, LoggerLevel ll)
{
    switch(ll){
        case(LoggerLevel(0)):
            logger -> info(msg);
            break;

        case(LoggerLevel(1)):
            logger -> debug(msg);
            break;

        case(LoggerLevel(2)):
            logger -> warn(msg);
            break;

        case(LoggerLevel(3)):
            logger -> error(msg);
            break;
    }
}

Logger& Logger::getInstance(const std::string &path)
{
    if (Logger::instance == nullptr) 
        instance = std::unique_ptr<Logger> (new Logger(path));
    return *instance;
}
