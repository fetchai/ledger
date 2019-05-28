#
# F E T C H   C O R E   B E N C H M A R K S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Benchmark Targets
# ------------------------------------------------------------------------------

add_executable(serialisation serialisation/main.cpp)
target_link_libraries(serialisation PRIVATE fetch-core fetch-testing)

add_fetch_gbench(core-random-benches fetch-core random/)
