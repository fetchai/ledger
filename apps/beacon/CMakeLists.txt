#
# BEACON
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(beacon)

setup_compiler()

add_executable(beacon_standalone main.cpp)
target_link_libraries(beacon_standalone PRIVATE fetch-version fetch-beacon)
