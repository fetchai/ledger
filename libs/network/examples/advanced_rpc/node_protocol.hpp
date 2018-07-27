#pragma once
#include "commands.hpp"
#include "node_functionality.hpp"
#include "vector_serialize.hpp"

#include "network/service/server.hpp"

class NodeToNodeProtocol : public NodeToNodeFunctionality, public fetch::service::Protocol
{
public:
  NodeToNodeProtocol(fetch::network::NetworkManager network_manager)
      : NodeToNodeFunctionality(network_manager), fetch::service::Protocol()
  {

    using namespace fetch::service;
    NodeToNodeFunctionality *controller = (NodeToNodeFunctionality *)this;
    this->Expose(PeerToPeerCommands::SEND_MESSAGE, controller,
                 &NodeToNodeFunctionality::SendMessage);
    this->Expose(PeerToPeerCommands::GET_MESSAGES, controller, &NodeToNodeFunctionality::messages);

    // Using the event feed that
    this->RegisterFeed(PeerToPeerFeed::NEW_MESSAGE, this);
  }
};
