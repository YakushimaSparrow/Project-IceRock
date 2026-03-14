# tests/CMakeLists.txt
# Вызывается из app/CMakeLists.txt через add_subdirectory.
# ROOT и PROTO_OUT уже заданы в include.cmake.

find_package(GTest REQUIRED)

add_executable(icerock_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/test_assets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_calculate.cpp"
)

# test1.cpp — если там есть свои тесты, тоже включаем
# Если test1.cpp пустой или не нужен — можно убрать строку ниже
target_sources(icerock_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/test1.cpp"
)

target_include_directories(icerock_tests PRIVATE
        "${ROOT}/include/calculate"
        "${ROOT}/include/logger"
        "${PROTO_OUT}"
        "${CMAKE_BINARY_DIR}"
)

target_link_libraries(icerock_tests PRIVATE
        calculate
        logger
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(icerock_tests)