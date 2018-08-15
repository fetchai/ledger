#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

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
