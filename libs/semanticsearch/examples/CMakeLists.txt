# ##################################################################################################
# F E T C H   S E M A N T I C   S E A R C H   E X A M P L E S
# ##################################################################################################
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-semanticsearch)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# -------------------------------------------------------------------------------
# Example Targets
# -------------------------------------------------------------------------------

setup_library_examples(fetch-semanticsearch)
