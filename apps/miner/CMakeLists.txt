################################################################################
# F E T C H   D E T A C H E D  M I N E R
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(miner)

# Compiler Configuration
setup_compiler()

file(GLOB_RECURSE headers *.hpp)
file(GLOB_RECURSE srcs *.cpp)
add_executable(detached_miner ${headers} ${srcs})
target_link_libraries(detached_miner PRIVATE fetch-core fetch-ledger fetch-miner fetch-crypto fetch-math)
target_include_directories(detached_miner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
