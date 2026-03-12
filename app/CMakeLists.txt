include_directories(..)
include_directories(../include)
include_directories(../src/ui)

add_executable(Icerock
        main.cpp
        ../src/ui/mainwindow.cpp
        ../src/ui/mainwindow.h
)

target_link_libraries(Icerock PRIVATE
        calculate
        logger
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
)