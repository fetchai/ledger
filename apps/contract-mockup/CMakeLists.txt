################################################################################
# F E T C H   C O N T R A C T   M O C K U P
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(fetch-constellation)

setup_compiler()

add_executable(contract-mockup
  main.cpp
)
target_link_libraries(contract-mockup PRIVATE fetch-ledger fetch-vm)

