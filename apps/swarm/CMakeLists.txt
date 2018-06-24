cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(swarm)

# Compiler Configuration
setup_compiler()

file(GLOB_RECURSE headers *.hpp)
file(GLOB_RECURSE srcs *.cpp)
add_executable(swarm ${headers} ${srcs})
target_link_libraries(swarm PRIVATE fetch-http fetch-network vendor-openssl)
target_include_directories(swarm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
