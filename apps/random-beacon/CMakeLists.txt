#
# F E T C H   R A N D O M   B E A C O N
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-constellation)

setup_compiler()

add_executable(random-beacon main.cpp)
target_link_libraries(random-beacon PRIVATE fetch-ledger fetch-dkg)
