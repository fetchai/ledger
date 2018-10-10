project(fetch-tx-generator)

setup_compiler()

add_executable(tx-generator
  main.cpp
)
target_link_libraries(tx-generator PRIVATE fetch-ledger fetch-core)

