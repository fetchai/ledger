################################################################################
# F E T C H   L E D G E R   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-ledger)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(proof_of_work_test fetch-ledger consensus/proof_of_work.cpp)

