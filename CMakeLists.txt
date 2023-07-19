cmake_minimum_required(VERSION 3.12)
project(lab9 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

link_directories(lib)

add_subdirectory(lib)
add_subdirectory(bin)
