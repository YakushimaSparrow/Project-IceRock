# cmake/include.cmake
# Подключается из app/CMakeLists.txt через include(include).
#
# ИСПРАВЛЕНИЕ sed для macOS:
# CMake убирает пустой аргумент "" поэтому "sed -i "" file" становится
# "sed -i file" что macOS интерпретирует как 'i' команду с file как аргументом.
# Решение: sed -i.bak "s/.../.../" file && rm file.bak

include(GenerateExportHeader)
cmake_policy(SET CMP0071 NEW)

if(NOT DEFINED ROOT)
    get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

# ------------------------------------------------------------------
# Qt6
# ------------------------------------------------------------------
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts OpenGLWidgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ------------------------------------------------------------------
# Threads
# ------------------------------------------------------------------
find_package(Threads REQUIRED)

# ------------------------------------------------------------------
# gRPC + Protobuf
# ------------------------------------------------------------------
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC    CONFIG REQUIRED)

set(_PROTO_LIB   protobuf::libprotobuf)
set(_GRPC_REFL   gRPC::grpc++_reflection)
set(_GRPC_LIB    gRPC::grpc++)
set(_PROTOC      $<TARGET_FILE:protobuf::protoc>)
set(_GRPC_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# ------------------------------------------------------------------
# Tinkoff investAPI proto
# ------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
        tinkoff_investapi
        GIT_REPOSITORY https://github.com/Tinkoff/investAPI.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(tinkoff_investapi)

set(_PROTO_DIR "${tinkoff_investapi_SOURCE_DIR}/src/docs/contracts")
if(NOT EXISTS "${_PROTO_DIR}")
    set(_PROTO_DIR "${tinkoff_investapi_SOURCE_DIR}")
endif()

file(GLOB_RECURSE _protos "${_PROTO_DIR}/*.proto")
if(NOT _protos)
    message(FATAL_ERROR "No .proto files found in: ${_PROTO_DIR}")
endif()

# ------------------------------------------------------------------
# Вспомогательный Python-скрипт для замены 'public' → 'public_'
# Используем Python вместо sed — он одинаково работает на всех платформах
# ------------------------------------------------------------------
set(PROTO_OUT "${CMAKE_BINARY_DIR}/proto_gen")
file(MAKE_DIRECTORY "${PROTO_OUT}")

# Пишем маленький python-скрипт в build-папку один раз
file(WRITE "${CMAKE_BINARY_DIR}/fix_proto.py" "
import sys, re
fname = sys.argv[1]
with open(fname, 'r', encoding='utf-8') as f:
    content = f.read()
content = content.replace('namespace public {', 'namespace public_ {')
content = content.replace('tinkoff::public::', 'tinkoff::public_::')
with open(fname, 'w', encoding='utf-8') as f:
    f.write(content)
")

set(PROTO_SOURCES "")

foreach(_proto ${_protos})
    get_filename_component(_n "${_proto}" NAME_WE)
    set(_ph "${PROTO_OUT}/${_n}.pb.h")
    set(_pc "${PROTO_OUT}/${_n}.pb.cc")
    set(_gh "${PROTO_OUT}/${_n}.grpc.pb.h")
    set(_gc "${PROTO_OUT}/${_n}.grpc.pb.cc")

    add_custom_command(
            OUTPUT  ${_pc} ${_ph} ${_gc} ${_gh}
            # Шаг 1: генерируем C++ из proto
            COMMAND ${_PROTOC}
            ARGS    --grpc_out   "${PROTO_OUT}"
            --cpp_out    "${PROTO_OUT}"
            --proto_path "${_PROTO_DIR}"
            -I           "${_PROTO_DIR}"
            --plugin=protoc-gen-grpc="${_GRPC_PLUGIN}"
            "${_proto}"
            # Шаг 2: заменяем 'public' -> 'public_' через Python (работает на macOS без проблем)
            COMMAND python3 "${CMAKE_BINARY_DIR}/fix_proto.py" "${_pc}"
            COMMAND python3 "${CMAKE_BINARY_DIR}/fix_proto.py" "${_ph}"
            COMMAND python3 "${CMAKE_BINARY_DIR}/fix_proto.py" "${_gc}"
            COMMAND python3 "${CMAKE_BINARY_DIR}/fix_proto.py" "${_gh}"
            DEPENDS "${_proto}"
            COMMENT "protoc + fix: ${_n}"
    )
    list(APPEND PROTO_SOURCES ${_pc} ${_gc})
endforeach()

# ------------------------------------------------------------------
# calculate
# ------------------------------------------------------------------
add_library(calculate STATIC
        "${ROOT}/src/calculate/R.cpp"
)
target_include_directories(calculate PUBLIC
        "${ROOT}/include/calculate"
)

# ------------------------------------------------------------------
# logger
# ------------------------------------------------------------------
find_package(spdlog REQUIRED)
add_library(logger STATIC
        "${ROOT}/src/logger/logger.cpp"
)
target_include_directories(logger PUBLIC
        "${ROOT}/include/logger"
)
target_link_libraries(logger PUBLIC spdlog::spdlog)

# ------------------------------------------------------------------
# parse
# ------------------------------------------------------------------
add_library(parse STATIC
        "${ROOT}/src/parse/Services.cpp"
        ${PROTO_SOURCES}
)
generate_export_header(parse
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/tinkoffinvestsdk_export.h"
)
target_include_directories(parse PUBLIC
        "${ROOT}/include/parse"
        "${PROTO_OUT}"
        "${CMAKE_BINARY_DIR}"
)
target_link_libraries(parse PUBLIC
        ${_GRPC_REFL}
        ${_GRPC_LIB}
        ${_PROTO_LIB}
        Threads::Threads
        calculate
        logger
)

# ------------------------------------------------------------------
# ui_library
# mainwindow.hpp лежит в src/ui/ — добавляем ОБА пути
# ------------------------------------------------------------------
add_library(ui_library STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/TinkoffBridge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/mainwindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/ui/TinkoffBridge.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/mainwindow.hpp
)
set_target_properties(ui_library PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
)
target_include_directories(ui_library PUBLIC
        "${ROOT}/include/ui"
        "${ROOT}/src/ui"
)
target_include_directories(ui_library PRIVATE
        "${ROOT}/include"
        "${ROOT}/include/parse"
        "${PROTO_OUT}"
        "${CMAKE_BINARY_DIR}"
)
target_link_libraries(ui_library PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Charts
        Qt6::OpenGLWidgets
        parse
)