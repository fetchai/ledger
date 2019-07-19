#
# E T C H
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(dkg)

setup_compiler()

add_executable(dkg_standalone main.cpp)
target_link_libraries(dkg_standalone PRIVATE fetch-version fetch-dkg)
