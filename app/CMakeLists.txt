cmake_minimum_required(VERSION 3.20)
project(IceRock VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# КОРЕНЬ ПРОЕКТА
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Подключаем cmake/include.cmake
list(APPEND CMAKE_MODULE_PATH "${ROOT}/cmake")
include(include)

# Тесты
enable_testing()
add_subdirectory("${ROOT}/tests" "${CMAKE_CURRENT_BINARY_DIR}/tests")

# Исполняемый файл IceRock
qt_add_executable(IceRock
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
set_target_properties(IceRock PROPERTIES
        MACOSX_BUNDLE TRUE
        AUTOMOC ON
)
target_link_libraries(IceRock PRIVATE
        ui_library
        parse
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)