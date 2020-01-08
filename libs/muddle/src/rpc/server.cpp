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

#include "muddle/rpc/server.hpp"

namespace fetch {
namespace muddle {
namespace rpc {

Server::Server(MuddleEndpoint &endpoint, uint16_t service, uint16_t channel)
  : endpoint_(endpoint)
  , service_(service)
  , channel_(channel)
  , subscription_(endpoint_.Subscribe(service, channel))
{
  // register the subscription with our handler
  subscription_->SetMessageHandler(this, &Server::OnMessage);
}

bool Server::DeliverResponse(ConstByteArray const &address, network::MessageBuffer const &data)
{
  FETCH_LOG_TRACE(LOGGING_NAME, "Server::DeliverResponse to: ", address.ToBase64(), " mdl ",
                  &endpoint_, " msg: ", data.ToHex());

  // send the message back to the server
  endpoint_.Send(address, service_, channel_, data);

  return true;
}

void Server::OnMessage(Packet const &packet, Address const &last_hop)
{
  // since this channel is shared between both the client and the server. We need to make sure that
  // we are only processing the messages that are relevant to us.
  if (!packet.IsExchange())
  {
    return;
  }

  FETCH_LOG_TRACE(LOGGING_NAME, "Server::OnMessage from: ", packet.GetSender().ToBase64(), " mdl ",
                  &endpoint_, " msg: 0x", packet.GetPayload().ToHex());

  service::CallContext context;
  context.sender_address      = packet.GetSender();
  context.transmitter_address = last_hop;
  context.MarkAsValid();

  // dispatch down to the core RPC level
  try
  {
    PushProtocolRequest(packet.GetSender(), packet.GetPayload(), context);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Recv message from: ", packet.GetSender().ToBase64(),
                    " on: ", packet.GetService(), ':', packet.GetChannel(), ':',
                    packet.GetMessageNum(), " -- ", ex.what());
  }
}

}  // namespace rpc
}  // namespace muddle
}  // namespace fetch
