add_executable(tutorial_aea_to_node_server tutorials/01_aea-to-node/node.cpp)
add_executable(tutorial_aea_to_node_client tutorials/01_aea-to-node/client.cpp)

add_executable(network_client network/client.cpp)
add_executable(network_server network/server.cpp)

add_executable(rpc_client rpc/client.cpp)
add_executable(rpc_server rpc/server.cpp)

add_executable(calculator script/calculator.cpp)

add_executable(sha256 crypto/sha256.cpp)


