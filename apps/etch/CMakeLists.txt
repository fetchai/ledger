#
# F E T C H   V M   L A N G
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-etch)

setup_compiler()

add_executable(etch main.cpp)
target_link_libraries(etch PRIVATE fetch-vm-modules)
