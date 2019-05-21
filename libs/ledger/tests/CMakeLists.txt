################################################################################
# F E T C H   L E D G E R   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ledger)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(fetch_ledger_chaincode_tests fetch-ledger chaincode)
add_fetch_test(fetch_ledger_executor_tests fetch-ledger executors SLOW)
target_include_directories(fetch_ledger_executor_tests PRIVATE chaincode)
add_fetch_test(fetch_ledger_consensus_tests fetch-ledger consensus)
add_fetch_test(fetch_ledger_chain_tests fetch-ledger chain)
