################################################################################
# F E T C H   T X  G E N E R A T O R
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(fetch-tx-generator)

setup_compiler()

add_executable(tx-generator
  main.cpp
)
target_link_libraries(tx-generator PRIVATE fetch-ledger fetch-core)

