################################################################################
# F E T C H   C R Y P T O   B E N C H M A R K S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-crypto)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Benchmark Targets
#-------------------------------------------------------------------------------

add_fetch_gbench(crypto-benchmarks fetch-crypto .)
