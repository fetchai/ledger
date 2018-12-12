################################################################################
# F E T C H   L E D G E R   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ledger)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gtest(chaincode_gtest fetch-ledger chaincode)
add_fetch_gtest(executors_gtest fetch-ledger executors)
target_include_directories(executors_gtest PRIVATE chaincode)
add_fetch_gtest(ledger_main_chain_gtest fetch-ledger consensus/gtest/)
add_fetch_gtest(ledger_gtest fetch-ledger gtest)
