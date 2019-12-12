#
# M U D D L E   N O D E
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(muddle-node)

setup_compiler()

add_executable(muddle-node main.cpp)
target_link_libraries(muddle-node PRIVATE fetch-muddle fetch-http)
