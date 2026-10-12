cmake_minimum_required(VERSION 3.20)

project(pse LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED CONFIG)

add_executable(pse src/main.cpp)

target_link_libraries(pse PRIVATE SDL3::SDL3)
