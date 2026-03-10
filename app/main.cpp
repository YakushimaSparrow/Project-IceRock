//просто запускает приложение
#include <iostream>
#include "R.hpp"
#include "logger.hpp"
#include <cassert>


//прописать >= 10 юнит-тестов
int main(){
    assert(1 == calculateMaxDragdown({1,2,3}));

    Logger::getInstance("logs/logbook.txt").log("Test", LoggerLevel::INFO);
    Logger::getInstance().log("Test2", LoggerLevel::INFO);
    return 0;
}