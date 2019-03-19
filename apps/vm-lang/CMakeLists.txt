################################################################################
# F E T C H   V M   L A N G
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-vmlang)

setup_compiler()

add_executable(vm-lang main.cpp)
target_link_libraries(vm-lang PRIVATE fetch-vm-modules)
