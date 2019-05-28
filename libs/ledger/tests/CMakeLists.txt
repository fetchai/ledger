#
# F E T C H   L E D G E R   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ledger)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(ledger-chaincode-tests fetch-ledger chaincode)
add_fetch_test(ledger-executor-tests
               fetch-ledger
               executors
               SLOW)
target_include_directories(ledger-executor-tests PRIVATE chaincode)
add_fetch_test(ledger-consensus-tests fetch-ledger consensus)
add_fetch_test(ledger-chain-tests fetch-ledger chain)

add_fetch_test(ledger-dag-tests fetch-ledger dag)

add_fetch_test(ledger-upow-tests fetch-ledger upow)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/upow/synergetic_test_contract.etch" 
               "${CMAKE_CURRENT_BINARY_DIR}/synergetic_test_contract.etch" COPYONLY)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/upow/synergetic_test_dag.dag" 
               "${CMAKE_CURRENT_BINARY_DIR}/synergetic_test_dag.dag" COPYONLY)
