cmake_minimum_required(VERSION 3.12)
project(game_server)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_CXX_STANDARD 17)
find_package(Threads)

link_libraries(${CMAKE_THREAD_LIBS_INIT})

add_executable(game_server game_server.cpp)
target_link_libraries(game_server ${CMAKE_THREAD_LIBS_INIT})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})