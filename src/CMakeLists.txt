
include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

#==============================================================================
# Qt
#==============================================================================
cmake_policy(SET CMP0071 NEW)

set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts OpenGLWidgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(${Qt6_INCLUDE_DIRS})
add_compile_definitions(${Qt6_DEFINITIONS})

set(UI_SOURCES
        ${CMAKE_SOURCE_DIR}/src/ui/mainwindow.cpp
)

set(UI_HEADERS
        ${CMAKE_SOURCE_DIR}/include/ui/mainwindow.hpp
)

# Теперь создаем библиотеку с исходниками и заголовками
add_library(ui_library STATIC ${UI_SOURCES} ${UI_HEADERS})

target_include_directories(ui_library PUBLIC
        ${CMAKE_SOURCE_DIR}/include/ui  # Публичный доступ для других библиотек
)

target_include_directories(ui_library PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/ui
        ${CMAKE_SOURCE_DIR}/src/ui
        ${Qt6_INCLUDE_DIRS}
)

target_link_libraries(ui_library PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Charts
        Qt6::OpenGLWidgets
)

# Добавляем свойства для Qt
set_target_properties(ui_library PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
)

#==============================================================================
# Настройка gRPC
#==============================================================================
find_package(Threads REQUIRED)

option(GRPC_FETCHCONTENT "Download and build gRPC from GitHub" OFF)

if(GRPC_FETCHCONTENT)
    include(FetchContent)
    FetchContent_Declare(
            grpc
            GIT_REPOSITORY https://github.com/grpc/grpc.git
            GIT_TAG v1.43.2
            GIT_SHALLOW TRUE
    )

    set(gRPC_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    set(gRPC_BUILD_BENCHMARKS OFF CACHE BOOL "Build benchmarks" FORCE)
    set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "Build C# plugin" FORCE)
    set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "Build Node plugin" FORCE)
    set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "Build ObjC plugin" FORCE)
    set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE BOOL "Build PHP plugin" FORCE)
    set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "Build Python plugin" FORCE)
    set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "Build Ruby plugin" FORCE)

    FetchContent_MakeAvailable(grpc)

    set(_PROTOBUF_LIBPROTOBUF libprotobuf)
    set(_REFLECTION grpc++_reflection)
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
    set(_GRPC_GRPCPP grpc++)

    if(CMAKE_CROSSCOMPILING)
        find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
    else()
        set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
    endif()

else()
    set(protobuf_MODULE_COMPATIBLE TRUE)

    find_package(Protobuf CONFIG REQUIRED)
    find_package(gRPC CONFIG REQUIRED)

    set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
    set(_REFLECTION gRPC::grpc++_reflection)
    set(_GRPC_GRPCPP gRPC::grpc++)

    if(CMAKE_CROSSCOMPILING)
        find_program(_PROTOBUF_PROTOC protoc)
        find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
    else()
        set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
        set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
    endif()
endif()

#==============================================================================
# Скачивание proto-файлов Tinkoff API
#==============================================================================
include(FetchContent)

set(TINKOFF_PROTO_TAG "main" CACHE STRING "Tag/branch of Tinkoff investAPI to use")

FetchContent_Declare(
        tinkoff_investapi
        GIT_REPOSITORY https://github.com/Tinkoff/investAPI.git
        GIT_TAG ${TINKOFF_PROTO_TAG}
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(tinkoff_investapi)

FetchContent_GetProperties(tinkoff_investapi)
if(NOT tinkoff_investapi_POPULATED)
    message(FATAL_ERROR "Failed to download tinkoff_investapi")
endif()

set(TINKOFF_PROTO_DIR "${tinkoff_investapi_SOURCE_DIR}/src/docs/contracts")

if(NOT EXISTS "${TINKOFF_PROTO_DIR}")
    set(TINKOFF_PROTO_DIR "${tinkoff_investapi_SOURCE_DIR}")
    if(NOT EXISTS "${TINKOFF_PROTO_DIR}")
        message(FATAL_ERROR "No valid proto directory found")
    endif()
endif()

#==============================================================================
# Поиск proto-файлов
#==============================================================================
file(GLOB_RECURSE tink_protos "${TINKOFF_PROTO_DIR}/*.proto")

if(NOT tink_protos)
    message(FATAL_ERROR "No .proto files found in ${TINKOFF_PROTO_DIR}")
endif()

list(LENGTH tink_protos proto_count)

#==============================================================================
# Генерация C++ кода из proto-файлов
#==============================================================================
foreach (tink_proto ${tink_protos})
    get_filename_component(tink_proto_name "${tink_proto}" NAME_WE)

    set(OUTPUT_PB_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/parse/${tink_proto_name}.pb.cc")
    set(OUTPUT_PB_HEADER "${CMAKE_CURRENT_BINARY_DIR}/parse/${tink_proto_name}.pb.h")
    set(OUTPUT_GRPC_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/parse/${tink_proto_name}.grpc.pb.cc")
    set(OUTPUT_GRPC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/parse/${tink_proto_name}.grpc.pb.h")

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/parse")

    add_custom_command(
            OUTPUT ${OUTPUT_PB_SOURCE} ${OUTPUT_PB_HEADER} ${OUTPUT_GRPC_SOURCE} ${OUTPUT_GRPC_HEADER}
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS
            --grpc_out "${CMAKE_CURRENT_BINARY_DIR}/parse"
            --proto_path ${TINKOFF_PROTO_DIR}
            --proto_path ${CMAKE_BINARY_DIR}/_deps/grpc-src/third_party/protobuf/src
            --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/parse"
            -I "${TINKOFF_PROTO_DIR}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${tink_proto}"
            COMMAND sed -i '' 's/namespace public {/namespace public_ {/g' ${OUTPUT_PB_SOURCE}
            COMMAND sed -i '' 's/tinkoff::public::/tinkoff::public_::/g' ${OUTPUT_PB_SOURCE}
            COMMAND sed -i '' 's/namespace public {/namespace public_ {/g' ${OUTPUT_PB_HEADER}
            COMMAND sed -i '' 's/tinkoff::public::/tinkoff::public_::/g' ${OUTPUT_PB_HEADER}
            COMMAND sed -i '' 's/namespace public {/namespace public_ {/g' ${OUTPUT_GRPC_SOURCE}
            COMMAND sed -i '' 's/tinkoff::public::/tinkoff::public_::/g' ${OUTPUT_GRPC_SOURCE}
            COMMAND sed -i '' 's/namespace public {/namespace public_ {/g' ${OUTPUT_GRPC_HEADER}
            COMMAND sed -i '' 's/tinkoff::public::/tinkoff::public_::/g' ${OUTPUT_GRPC_HEADER}
            DEPENDS "${tink_proto}"
            COMMENT "Generating gRPC code from ${tink_proto_name}.proto"
    )

    list(APPEND PROTO_GENERATED_SOURCES ${OUTPUT_PB_SOURCE} ${OUTPUT_GRPC_SOURCE})
    list(APPEND PROTO_GENERATED_HEADERS ${OUTPUT_PB_HEADER} ${OUTPUT_GRPC_HEADER})
endforeach()

#==============================================================================
# calculate
#==============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/src/calculate/R.cpp)
    add_library(calculate ${CMAKE_SOURCE_DIR}/src/calculate/R.cpp)
    target_include_directories(calculate PUBLIC ${CMAKE_SOURCE_DIR}/include/calculate)
else()
    message(FATAL_ERROR "calculate source not found: ${CMAKE_SOURCE_DIR}/src/calculate/R.cpp")
endif()

#==============================================================================
# logger
#==============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/src/logger/logger.cpp)
    find_package(spdlog REQUIRED)

    add_library(logger ${CMAKE_SOURCE_DIR}/src/logger/logger.cpp)
    target_include_directories(logger PUBLIC ${CMAKE_SOURCE_DIR}/include/logger)
    target_link_libraries(logger PUBLIC spdlog::spdlog)
else()
    message(FATAL_ERROR "logger source not found: ${CMAKE_SOURCE_DIR}/src/logger/logger.cpp")
endif()

#==============================================================================
# parse (с включенными gRPC и сгенерированным кодом)
#==============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/src/parse/Services.cpp)
    set(PARSE_SOURCES
            ${CMAKE_SOURCE_DIR}/src/parse/Services.cpp
            ${PROTO_GENERATED_SOURCES}
    )

    add_library(parse ${PARSE_SOURCES})

    generate_export_header(parse)

    target_include_directories(parse PUBLIC
            ${CMAKE_SOURCE_DIR}/include/parse
            ${CMAKE_CURRENT_BINARY_DIR}/parse
            ${CMAKE_CURRENT_BINARY_DIR}
    )

    target_link_libraries(parse PUBLIC
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF}
            Threads::Threads
            calculate
            logger
    )
else()
    message(FATAL_ERROR "parse source not found: ${CMAKE_SOURCE_DIR}/src/parse/Services.cpp")
endif()

#==============================================================================
# Основная библиотека SDK (если есть файлы)
#==============================================================================
file(GLOB TARGET_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/services/*.cpp"
)

if(TARGET_SRC)
    add_library(TinkoffInvestSDK SHARED ${TARGET_SRC})

    generate_export_header(TinkoffInvestSDK)

    target_include_directories(TinkoffInvestSDK
            PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(TinkoffInvestSDK
            PRIVATE
            parse
            calculate
            logger
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF}
            Threads::Threads
    )
endif()