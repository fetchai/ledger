#
# S T O R A G E   U N I T   C L I
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(storage-unit-cli)

setup_compiler()

add_executable(tx-ctl
               tx_ctl.cpp
               tx_storage_client.cpp
               tx_storage_client.hpp
               tx_storage_tool.cpp
               tx_storage_tool.hpp)
target_link_libraries(tx-ctl PRIVATE fetch-ledger)

add_executable(blk-ctl
               blk_ctl.cpp
               block_storage_client.cpp
               block_storage_client.hpp
               block_storage_tool.cpp
               block_storage_tool.hpp)
target_link_libraries(blk-ctl PRIVATE fetch-ledger)
