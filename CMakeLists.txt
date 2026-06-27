cmake_minimum_required(VERSION 3.10)
project(riscv_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Wextra")

file(GLOB SOURCES "src/*.cpp")

add_executable(riscv-sim ${SOURCES})
