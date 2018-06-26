cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(demonode)

# Compiler Configuration
setup_compiler()

file(GLOB_RECURSE headers *.hpp)
file(GLOB_RECURSE srcs *.cpp)
add_executable(demonode ${headers} ${srcs})
target_link_libraries(demonode PRIVATE fetch-http fetch-network vendor-openssl)
target_include_directories(demonode PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${FETCH_ROOT_DIR}/network/include ${FETCH_ROOT_DIR}/apps/swarm )
