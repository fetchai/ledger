#
# M U D D L E   T E S T   N E T W O R K
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(muddle-probe)

setup_compiler()

add_executable(muddle-test-net main.cpp)
target_link_libraries(muddle-test-net PRIVATE fetch-muddle fetch-constellation)
