cmake_minimum_required(VERSION 2.8)

project(d64 C)

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE )
endif()

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Werror -pedantic --std=c99 -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -Wextra -Werror -pedantic --std=c99 -s")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

include_directories(src)

file(GLOB SOURCES "src/*.c")

add_executable(d64 ${SOURCES})
