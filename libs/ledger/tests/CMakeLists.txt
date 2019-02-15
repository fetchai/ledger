################################################################################
# F E T C H   L E D G E R   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ledger)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(chaincode_gtest fetch-ledger chaincode SLOW)
add_fetch_test(executors_gtest fetch-ledger executors SLOW)
target_include_directories(executors_gtest PRIVATE chaincode)
add_fetch_test(ledger_main_chain_gtest fetch-ledger consensus)
add_fetch_test(ledger-chain-tests fetch-ledger chain)

