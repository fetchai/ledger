#ifndef NODE_PROTOCOL_HPP
#define NODE_PROTOCOL_HPP
#include"vector_serialize.hpp"
#include"node_functionality.hpp"
#include"commands.hpp"

#include"rpc/service_server.hpp"

class NodeProtocol : public NodeFunctionality, public fetch::rpc::Protocol { 
public:
  NodeProtocol() : NodeFunctionality(),  fetch::rpc::Protocol() {

    using namespace fetch::rpc;
    auto send_msg =  new CallableClassMember<NodeFunctionality, void(std::string)>(this, &NodeFunctionality::SendMessage);
    auto get_msgs =  new CallableClassMember<NodeFunctionality, std::vector< std::string >() >(this, &NodeFunctionality::messages);    
      
    this->Expose(PeerToPeerCommands::SEND_MESSAGE, send_msg);
    this->Expose(PeerToPeerCommands::GET_MESSAGES, get_msgs); 
  }
};

#endif
