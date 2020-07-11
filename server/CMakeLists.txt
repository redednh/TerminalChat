cmake_minimum_required(VERSION 3.5)

project(chat VERSION 0.0.1)

add_executable(server main.cpp server.cpp server.h)

find_package (Threads)

set(
    CMAKE_CXX_STANDARD ON
    CXX_STANDARD_REQUIRED 17
)

target_link_libraries (server Threads::Threads)
