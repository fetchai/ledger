#
# E T C H
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(etch)

setup_compiler()

add_executable(etch main.cpp)
target_link_libraries(etch PRIVATE fetch-vm-modules)
