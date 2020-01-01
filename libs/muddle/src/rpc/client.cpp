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

#include "muddle/rpc/client.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace fetch {
namespace muddle {
namespace rpc {

Client::Client(std::string name, MuddleEndpoint &endpoint, uint16_t service, uint16_t channel)
  : name_(std::move(name))
  , endpoint_(endpoint)
  , subscription_(endpoint_.Subscribe(service, channel))
  , network_id_(endpoint.network_id())
  , service_(service)
  , channel_(channel)
{
  // set the message handler
  subscription_->SetMessageHandler(this, &Client::OnMessage);
}

bool Client::DeliverRequest(muddle::Address const &address, network::MessageBuffer const &data)
{
  FETCH_LOG_TRACE(LOGGING_NAME, "Client::DeliverRequest to: ", address.ToBase64(), " mdl ",
                  &endpoint_, " msg: 0x", data.ToHex());

  bool success{false};

  try
  {
    endpoint_.Send(address, service_, channel_, data, MuddleEndpoint::OPTION_EXCHANGE);
    success = true;
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Error sending RPC message: ", e.what());
    throw e;
  }

  return success;
}

void Client::OnMessage(Packet const &packet, Address const &last_hop)
{
  FETCH_UNUSED(last_hop);

  // since this channel is shared between both the client and the server. We need to make sure that
  // we are only processing the messages that are relevant to us.
  if (packet.IsExchange())
  {
    return;
  }

  FETCH_LOG_TRACE(LOGGING_NAME, "Client::OnMessage from: 0x", packet.GetSender().ToBase64(),
                  " mdl ", &endpoint_, " msg: 0x", packet.GetPayload().ToHex(), " ctx: ", this);

  try
  {
    ProcessServerMessage(packet.GetPayload());
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Error processing server message: ", ex.what());
  }
}

}  // namespace rpc
}  // namespace muddle
}  // namespace fetch
