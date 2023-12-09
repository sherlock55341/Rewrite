cmake_minimum_required(VERSION 3.18)

project(rewrite)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)

set(UTIL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utilitys)

add_subdirectory(src)
add_subdirectory(utilitys)
