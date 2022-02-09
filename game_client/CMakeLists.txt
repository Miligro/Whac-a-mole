cmake_minimum_required(VERSION 3.1)

project(game_client)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

set(SRCS
    main.cpp
    mygame.ui
    mygame.cpp
    mygame.h
    menuwindow.cpp
    menuwindow.h
    menuwindow.ui
    gameboard.cpp
    gameboard.h
    gameboard.ui
)

add_executable(game_client ${SRCS})

target_link_libraries(game_client Qt5::Widgets Qt5::Network)
