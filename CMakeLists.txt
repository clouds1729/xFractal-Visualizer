cmake_minimum_required(VERSION 3.16)
project(xFractalVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_executable(xfractal_visualizer
    SDLP1/main.cpp
)

target_link_libraries(xfractal_visualizer PRIVATE SDL2::SDL2)

if (MSVC)
    target_compile_options(xfractal_visualizer PRIVATE /W4)
else()
    target_compile_options(xfractal_visualizer PRIVATE -Wall -Wextra -Wpedantic)
endif()
