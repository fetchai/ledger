################################################################################
# F E T C H   C R Y P T O   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-crypto)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(ecdsa_test fetch-crypto ecdsa.cpp DISABLED)
add_fetch_test(sha256_test fetch-crypto sha256.cpp)

add_fetch_gtest(fetch-crypto-gtests fetch-crypto gtests)

