add_executable(benchmark_serialization serialization/main.cpp)

add_executable(benchmark_rpc_server_localhost rpc/localhost.cpp)
add_executable(benchmark_rpc_push_localhost   rpc/localhost_push.cpp)

add_executable(benchmark_rpc_server_remote    rpc/remote.cpp)

add_executable(benchmark_matrix math/linalg/matrix.cpp)
