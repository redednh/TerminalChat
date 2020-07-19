cmake_minimum_required(VERSION 3.5)

project(chat VERSION 0.0.2)

file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/../commandServer.h)
add_executable(client main.cpp client.cpp client.h ${HEADER_FILES})

find_package (Threads)

set(
    CMAKE_CXX_STANDARD ON
    CXX_STANDARD_REQUIRED 17
)

target_link_libraries (client Threads::Threads)
