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
#include "core/assert.hpp"
#include "network/service/client.hpp"
#include "network/service/publication_feed.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

class NodeToNodeFunctionality : public fetch::service::HasPublicationFeed
{
public:
  using client_type = fetch::service::ServiceClient;

  NodeToNodeFunctionality(fetch::network::NetworkManager network_manager)
    : network_manager_(network_manager)
  {}

  void Tick() { this->Publish(PeerToPeerFeed::NEW_MESSAGE, "tick"); }

  void Tock() { this->Publish(PeerToPeerFeed::NEW_MESSAGE, "tock"); }

  void SendMessage(std::string message)
  {
    std::cout << "Received message: " << message << std::endl;
    messages_.push_back(message);
  }

  std::vector<std::string> messages() { return messages_; }

  void Connect(std::string host, uint16_t port)
  {
    std::cout << "Node connecting to " << host << " on " << port << std::endl;

    this->Publish(PeerToPeerFeed::CONNECTING, host, port);

    fetch::network::TCPClient connection(network_manager_);
    connection.Connect(host, port);

    connections_.push_back(std::make_shared<client_type>(connection, network_manager_));
  }

private:
  fetch::network::NetworkManager            network_manager_;
  std::vector<std::string>                  messages_;
  std::vector<std::shared_ptr<client_type>> connections_;
};
