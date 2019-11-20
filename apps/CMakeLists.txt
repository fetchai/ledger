#
# F E T C H   A P P S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-apps)

add_subdirectory(constellation)
add_subdirectory(etch)
add_subdirectory(tx-generator)
add_subdirectory(muddle-node)
add_subdirectory(muddle-probe)
add_subdirectory(chain-tool)
add_subdirectory(muddle-test-network)


if (NOT APPLE)
  add_subdirectory(oef-core)
  add_subdirectory(oef-search)
endif ()
