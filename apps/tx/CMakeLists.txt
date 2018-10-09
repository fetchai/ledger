project(fetch-constellation)

setup_compiler()

add_executable(tx_generator
  main.cpp
)
target_link_libraries(constellation PRIVATE fetch-ledger)
