add_executable(Icerock main.cpp)
target_link_libraries(Icerock PUBLIC
        calculate
        logger
        parse
        ui_library
)