################################################################################
# F E T C H   L E D G E R   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1)
project(fetch-ledger)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-ledger)
target_link_libraries(fetch-ledger INTERFACE fetch-network fetch-http fetch-crypto fetch-math)

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(proof_of_work_test fetch-ledger tests/consensus/proof_of_work.cpp)

#-------------------------------------------------------------------------------
# Example Targets
#-------------------------------------------------------------------------------

setup_library_examples(fetch-ledger)
