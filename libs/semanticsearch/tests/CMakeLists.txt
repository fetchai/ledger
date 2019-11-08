#
# F E T C H   S E M A N T I C   S E A R C H   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-semanticsearch)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(semanticsearch-unit-tests fetch-semanticsearch unit/)
