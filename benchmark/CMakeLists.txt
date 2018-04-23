add_executable(benchmark_serialization serialization/main.cpp)

add_executable(benchmark_rpc_server rpc/main.cpp)
add_executable(benchmark_rpc_push rpc/main_push.cpp)

add_executable(benchmark_matrix math/linalg/matrix.cpp)
