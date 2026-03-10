add_library(calculate ${CMAKE_SOURCE_DIR}/src/calculate/R.cpp)
target_include_directories(calculate PUBLIC ${CMAKE_SOURCE_DIR}/include/calculate)

add_library(logger ${CMAKE_SOURCE_DIR}/src/logger/logger.cpp)
target_include_directories(logger PUBLIC ${CMAKE_SOURCE_DIR}/include/logger)
target_link_libraries(logger PUBLIC spdlog::spdlog)

add_library(TradingViewParser ${CMAKE_SOURCE_DIR}/src/parse/TVparser.cpp)
target_include_directories(TradingViewParser PUBLIC ${CMAKE_SOURCE_DIR}/include/parse)