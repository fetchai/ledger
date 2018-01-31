add_executable(p2p_node ledger/peer-to-peer/main.cpp)
add_executable(simple_ledger_node ledger/simple-ledger/node.cpp)


add_executable(network_client network/client.cpp)
add_executable(network_server network/server.cpp)

add_executable(rpc_client rpc/client.cpp)
add_executable(rpc_server rpc/server.cpp)

add_executable(calculator script/calculator.cpp)

add_executable(sha256 crypto/sha256.cpp)


## Legacy
# add_executable(tutorial_rc_to_node_server tutorials/01_rc-to-node/node.cpp)
# add_executable(tutorial_rc_to_node_client tutorials/01_rc-to-node/client.cpp)

# add_executable(tutorial_rc_to_node_p2_server tutorials/02_rc-to-node-p2/node.cpp)
# add_executable(tutorial_rc_to_node_p2_client tutorials/02_rc-to-node-p2/client.cpp)

