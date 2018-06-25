#ifndef NODE_PROTOCOL_HPP
#define NODE_PROTOCOL_HPP
#include"vector_serialize.hpp"
#include"node_functionality.hpp"
#include"commands.hpp"

#include"network/service/server.hpp"

class NodeToNodeProtocol : public NodeToNodeFunctionality, public fetch::service::Protocol { 
public:
  NodeToNodeProtocol(fetch::network::ThreadManager thread_manager) :
    NodeToNodeFunctionality(thread_manager),
    fetch::service::Protocol() {

    using namespace fetch::service;
    NodeToNodeFunctionality* controller = (NodeToNodeFunctionality*) this;        
    this->Expose(PeerToPeerCommands::SEND_MESSAGE, controller,  &NodeToNodeFunctionality::SendMessage);
    this->Expose(PeerToPeerCommands::GET_MESSAGES, controller, &NodeToNodeFunctionality::messages);

    // Using the event feed that
    this->RegisterFeed( PeerToPeerFeed::NEW_MESSAGE, this  );

  }
};

#endif
