#
# F E T C H   M U D D L E S C O P E
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-muddlescope)

setup_compiler()

add_executable(muddlescope main.cpp scope_client.cpp)
target_link_libraries(muddlescope PRIVATE fetch-network)
