#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "core/logger.hpp"
#include "network/service/server.hpp"
#include "network/tcp/tcp_server.hpp"
#include "protocols/fetch_protocols.hpp"
#include "protocols/subscribe/protocol.hpp"

#include <memory>

namespace fetch {
namespace subscribe {

/*
 * Class containing one or more protocols, thus defining a service. Inherits
 * from ServiceServer to provide network functionality
 */
class SubscribeService : public service::ServiceServer<fetch::network::TCPServer>
{
public:
  static constexpr char const *LOGGING_NAME = "SubscribeService";

  /*
   * Constructor for SubscribeService, will create a server to respond to rpc
   * calls
   */
  SubscribeService(fetch::network::NetworkManager tm, uint16_t tcpPort)
    : ServiceServer(tcpPort, tm)
  {
    // Macro used for debugging
    LOG_STACK_TRACE_POINT;

    // Prints when compiled in debug mode. Options: logger.Debug logger.Info
    // logger.Error
    FETCH_LOG_DEBUG(LOGGING_NAME, "Constructing test node service with TCP port: ", tcpPort);

    // We construct our node, and attach it to the protocol
    subscribeProto_ = std::make_unique<protocols::SubscribeProtocol>();

    // We 'Add' these protocols
    this->Add(protocols::FetchProtocols::SUBSCRIBE_PROTO, subscribeProto_.get());
  }

  // We can use this to send messages to interested nodes
  void SendMessage(std::string const &mes)
  {
    subscribeProto_->SendMessage(mes);
  }

private:
  std::unique_ptr<protocols::SubscribeProtocol> subscribeProto_;
};
}  // namespace subscribe
}  // namespace fetch
