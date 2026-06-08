cmake_minimum_required(VERSION 3.16)
project(http_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(include utils)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(http_server ${SOURCES})

target_link_libraries(http_server pthread)
