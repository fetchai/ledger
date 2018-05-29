add_executable(benchmark_serialization serialization/main.cpp)

add_executable(benchmark_rpc    rpc/main.cpp)

add_executable(benchmark_network network/network_benchmark.cpp network/node_basic.hpp)

add_executable(benchmark_matrix math/linalg/matrix.cpp)
