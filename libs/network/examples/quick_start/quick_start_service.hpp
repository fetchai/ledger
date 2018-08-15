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

#include "./node.hpp"
#include "./protocols/fetch_protocols.hpp"  // defines QUICK_START enum
#include "core/logger.hpp"
#include "network/service/server.hpp"
#include <memory>

namespace fetch {
namespace quick_start {

/*
 * Class containing one or more protocols, thus defining a service. Inherits
 * from ServiceServer to provide network functionality
 */
class QuickStartService : public service::ServiceServer<fetch::network::TCPServer>
{
public:
  /*
   * Constructor for QuickStartService, will create a server to respond to rpc
   * calls
   */
  QuickStartService(fetch::network::NetworkManager tm, uint16_t tcpPort)
    : ServiceServer(tcpPort, tm)
  {
    // Macro used for debugging
    LOG_STACK_TRACE_POINT;

    // Prints when compiled in debug mode. Options: logger.Debug logger.Info
    // logger.Error
    fetch::logger.Debug("Constructing test node service with TCP port: ", tcpPort);

    // We construct our node, and attach it to the protocol
    node_               = std::make_shared<Node>(tm);
    quickStartProtocol_ = std::make_unique<protocols::QuickStartProtocol<Node>>(node_);

    // We 'Add' these protocols under our QUICK_START enum
    this->Add(protocols::QuickStartProtocols::QUICK_START, quickStartProtocol_.get());
  }

  // We can use this to send messages from node to node
  void sendMessage(std::string const &mes, uint16_t port) { node_->sendMessage(mes, port); }

private:
  std::shared_ptr<Node>                                node_;
  std::unique_ptr<protocols::QuickStartProtocol<Node>> quickStartProtocol_;
};
}  // namespace quick_start
}  // namespace fetch
