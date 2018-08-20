project(fetch-constellation)

setup_compiler()

add_executable(constellation
  main.cpp
  constellation.cpp
  constellation.hpp
)
target_link_libraries(constellation PRIVATE fetch-ledger fetch-miner)

