cmake_minimum_required(VERSION 3.0)

project(emu)

find_package(SDL2 REQUIRED)

find_package(Boost 1.68 REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-conversion -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-variable-declarations -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Weverything -Wno-conversion -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-variable-declarations -pedantic")

add_subdirectory(emulator)
add_subdirectory(invaders)
