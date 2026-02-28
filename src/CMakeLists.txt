add_library(calculate ${CMAKE_SOURCE_DIR}/src/calculate/R.cpp)

target_include_directories(calculate PUBLIC ${CMAKE_SOURCE_DIR}/include/calculate)