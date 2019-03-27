cmake_minimum_required(VERSION 2.8)

project(d64 C)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic --std=c99 -O2 -s")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

include_directories(src)

file(GLOB SOURCES "src/*.c")

add_executable(d64 ${SOURCES})
