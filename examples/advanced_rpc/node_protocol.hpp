#ifndef NODE_PROTOCOL_HPP
#define NODE_PROTOCOL_HPP
#include"vector_serialize.hpp"
#include"node_functionality.hpp"
#include"commands.hpp"

#include"service/server.hpp"

class NodeToNodeProtocol : public NodeToNodeFunctionality, public fetch::service::Protocol { 
public:
  NodeToNodeProtocol(fetch::network::ThreadManager *thread_manager) :
    NodeToNodeFunctionality(thread_manager),
    fetch::service::Protocol() {

    using namespace fetch::service;
    auto send_msg =  new CallableClassMember<NodeToNodeFunctionality, void(std::string)>(this, &NodeToNodeFunctionality::SendMessage);
    auto get_msgs =  new CallableClassMember<NodeToNodeFunctionality, std::vector< std::string >() >(this, &NodeToNodeFunctionality::messages);    
      
    this->Expose(PeerToPeerCommands::SEND_MESSAGE, send_msg);
    this->Expose(PeerToPeerCommands::GET_MESSAGES, get_msgs);

    // Using the event feed that
    this->RegisterFeed( PeerToPeerFeed::NEW_MESSAGE, this  );

  }
};

#endif
