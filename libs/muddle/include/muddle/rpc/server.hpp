#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "network/service/call_context.hpp"
#include "network/service/server_interface.hpp"
#include "network/tcp/tcp_server.hpp"

#include <array>
#include <tuple>
#include <unordered_map>

namespace fetch {
namespace muddle {
namespace rpc {

class Server : public service::ServiceServerInterface
{
public:
  using ProtocolId      = service::ProtocolHandlerType;
  using Protocol        = service::Protocol;
  using SubscriptionPtr = MuddleEndpoint::SubscriptionPtr;
  using SubscriptionMap = std::unordered_map<ProtocolId, SubscriptionPtr>;

  static constexpr char const *LOGGING_NAME = "MuddleRpcServer";

  // Construction / Destruction
  Server(MuddleEndpoint &endpoint, uint16_t service, uint16_t channel);
  Server(Server const &) = delete;
  Server(Server &&)      = delete;
  ~Server() override     = default;

  // Operators
  Server &operator=(Server const &) = delete;
  Server &operator=(Server &&) = delete;

protected:
  bool DeliverResponse(ConstByteArray const &address, network::MessageBuffer const &data) override;

private:
  void OnMessage(Packet const &packet, Address const &last_hop);

  MuddleEndpoint &endpoint_;
  uint16_t const  service_;
  uint16_t const  channel_;
  SubscriptionPtr subscription_;
};

}  // namespace rpc
}  // namespace muddle
}  // namespace fetch
