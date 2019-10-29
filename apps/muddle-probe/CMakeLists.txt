#
# M U D D L E   P R O B E
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(muddle-probe)

setup_compiler()

add_executable(muddle-probe main.cpp)
target_link_libraries(muddle-probe PRIVATE fetch-muddle)
