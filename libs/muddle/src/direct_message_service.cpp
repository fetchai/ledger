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

#include "core/service_ids.hpp"
#include "direct_message_service.hpp"
#include "muddle_logging_name.hpp"
#include "muddle_register.hpp"
#include "peer_list.hpp"
#include "router.hpp"
#include "routing_message.hpp"

namespace fetch {
namespace muddle {
namespace {

using fetch::byte_array::ConstByteArray;

constexpr char const *BASE_NAME = "DirectHandler";

template <typename T>
ConstByteArray EncodePayload(T const &msg)
{
  ConstByteArray payload{};
  try
  {
    serializers::MsgPackSerializer serializer;
    serializer << msg;

    payload = serializer.data();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(BASE_NAME, "Unable to encode payload: ", ex.what());
  }

  return payload;
}

template <typename T>
bool ExtractPayload(ConstByteArray const &payload, T &msg)
{
  bool success{false};

  try
  {
    serializers::MsgPackSerializer serializer{payload};
    serializer >> msg;

    success = true;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(BASE_NAME, "Unable to extract payload: ", ex.what());
  }

  return success;
}

/**
 * Generate a direct packet with a payload
 *
 * @param from The address of the sender
 * @param service The service identifier
 * @param channel The channel identifier
 * @return The packet
 */
template <typename T>
Router::PacketPtr FormatDirect(Packet::Address const &from, NetworkId const &network,
                               uint16_t service, uint16_t channel, T const &msg,
                               bool exchange = false)
{
  auto packet = std::make_shared<Packet>(from, network.value());
  packet->SetService(service);
  packet->SetChannel(channel);
  packet->SetDirect(true);
  packet->SetExchange(exchange);
  packet->SetPayload(EncodePayload(msg));

  return packet;
}

}  // namespace

DirectMessageService::DirectMessageService(Address address, Router &router, MuddleRegister &reg,
                                           PeerConnectionList & /*peers*/)
  : address_{std::move(address)}
  , name_{GenerateLoggingName(BASE_NAME, router.network_id())}
  , router_{router}
  , register_{reg}
{
  router_.SetDirectHandler(
      [this](Handle handle, PacketPtr packet) { OnDirectMessage(handle, packet); });
}

void DirectMessageService::InitiateConnection(Handle handle)
{
  FETCH_LOG_TRACE(logging_name_, "Init. Connection (conn: ", handle, ")");

  // format the message
  RoutingMessage msg{};
  msg.type = RoutingMessage::Type::PING;

  // send the message to the connection
  SendMessageToConnection(handle, msg);
}

void DirectMessageService::SignalConnectionLeft(Handle handle)
{
  FETCH_LOCK(lock_);

  for (auto it = reservations_.begin(); it != reservations_.end();)
  {
    if (it->second == handle)
    {
      it = reservations_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

template <typename T>
void DirectMessageService::SendMessageToConnection(Handle handle, T const &msg, bool exchange)
{
  // send the response
  router_.SendToConnection(
      handle, router_.Sign(FormatDirect(router_.address_, router_.network_id_, SERVICE_MUDDLE,
                                        CHANNEL_ROUTING, msg, exchange)));
}

void DirectMessageService::OnDirectMessage(Handle handle, PacketPtr const &packet)
{
  FETCH_LOCK(lock_);

  // inform the register about the address update for the connection (always applicable if not
  // always used for routing)
  if (SERVICE_MUDDLE == packet->GetService())
  {
    if (CHANNEL_ROUTING == packet->GetChannel())
    {
      // extract the message from the packet
      RoutingMessage msg;
      if (!ExtractPayload(packet->GetPayload(), msg))
      {
        FETCH_LOG_WARN(logging_name_, "Unable to extract routing message payload (conn: ", handle,
                       ")");
        return;
      }

      // dispatch the routing message
      OnRoutingMessage(handle, packet, msg);
    }
  }
}

void DirectMessageService::OnRoutingMessage(Handle handle, PacketPtr const &packet,
                                            RoutingMessage const &msg)
{
  FETCH_LOG_TRACE(logging_name_, "OnRoutingMessage");

  switch (msg.type)
  {
  case RoutingMessage::Type::PING:
    OnRoutingPing(handle, packet, msg);
    break;
  case RoutingMessage::Type::PONG:
    OnRoutingPong(handle, packet, msg);
    break;
  case RoutingMessage::Type::ROUTING_REQUEST:
    OnRoutingRequest(handle, packet, msg);
    break;
  case RoutingMessage::Type::ROUTING_ACCEPTED:
    break;
  default:
    break;
  }
}

void DirectMessageService::OnRoutingPing(Handle handle, PacketPtr const &packet,
                                         RoutingMessage const &msg)
{
  FETCH_LOG_TRACE(logging_name_, "OnRoutingPing (conn: ", handle, ")");
  FETCH_UNUSED(packet);
  FETCH_UNUSED(msg);

  RoutingMessage response{};
  response.type = RoutingMessage::Type::PONG;

  // keep a record of this identity in the connection data

  // send the response to the server
  SendMessageToConnection(handle, response);
}

void DirectMessageService::OnRoutingPong(Handle handle, PacketPtr const &packet,
                                         RoutingMessage const &msg)
{
  FETCH_LOG_TRACE(logging_name_, "OnRoutingPong (conn: ", handle, ")");
  FETCH_UNUSED(msg);

  // determine if we have not routed to this connection yet
  Handle     previous_handle{0};
  auto const status = UpdateReservation(packet->GetSender(), handle, &previous_handle);
  if (UpdateStatus::ADDED == status)
  {
    // format the message
    RoutingMessage response{};
    response.type = RoutingMessage::Type::ROUTING_REQUEST;

    // send the message to the connection
    SendMessageToConnection(handle, response);
  }
  else if (UpdateStatus::REPLACED == status)
  {
    // format the message
    RoutingMessage response{};

    // send the connection request
    response.type = RoutingMessage::Type::ROUTING_REQUEST;
    SendMessageToConnection(handle, response);
  }
  else
  {
    FETCH_LOG_WARN(logging_name_, "Pong Reserve failed for conn: ", handle,
                   " status: ", ToString(status));
  }

  FETCH_LOG_TRACE(logging_name_, "OnRoutingPong (conn: ", handle, ") complete");
}

void DirectMessageService::OnRoutingRequest(Handle handle, PacketPtr const &packet,
                                            RoutingMessage const &msg)
{
  FETCH_UNUSED(msg);
  FETCH_LOG_TRACE(logging_name_, "OnRoutingRequest (conn: ", handle, ")");

  RoutingMessage response{};
  response.type = RoutingMessage::Type::DISCONNECT_REQUEST;

  Handle     previous_handle{0};
  auto const status = UpdateReservation(packet->GetSender(), handle, &previous_handle);

  FETCH_LOG_TRACE(logging_name_, "Req. Reserve for conn: ", handle, " status: ", ToString(status));

  if ((UpdateStatus::REPLACED == status) || (UpdateStatus::ADDED == status))
  {
    // in the case of the replaced connection, we need to inform the replaced connection of the
    // disconnect request
    if (UpdateStatus::REPLACED == status)
    {
      SendMessageToConnection(previous_handle, response, true);
    }

    // set the accepted flag for the main response
    response.type = RoutingMessage::Type::ROUTING_ACCEPTED;
  }
  else
  {
    FETCH_LOG_WARN(logging_name_, "Requesting connection disconnect (conn: ", handle, ")");
  }

  // send back the response
  SendMessageToConnection(handle, response, true);
}

char const *DirectMessageService::ToString(UpdateStatus status)
{
  char const *text = "Unknown";

  switch (status)
  {
  case UpdateStatus::NO_CHANGE:
    text = "No Change";
    break;

  case UpdateStatus::ADDED:
    text = "Added";
    break;

  case UpdateStatus::REPLACED:
    text = "Replaced";
    break;

  case UpdateStatus::DUPLICATE:
    text = "Duplicate";
    break;
  }

  return text;
}

DirectMessageService::UpdateStatus DirectMessageService::UpdateReservation(Address const &address,
                                                                           Handle         handle,
                                                                           Handle *previous_handle)
{
  UpdateStatus status{UpdateStatus::DUPLICATE};

  auto it = reservations_.find(address);
  if (it == reservations_.end())
  {
    reservations_.emplace(address, handle);

    status = UpdateStatus::ADDED;
  }
  else
  {
    if (it->second == handle)
    {
      status = UpdateStatus::NO_CHANGE;
    }
    else
    {
      auto conn = register_.LookupConnection(handle).lock();
      if (conn)
      {
        // determine the connection orientation
        bool const is_outgoing = conn->Type() == network::AbstractConnection::TYPE_OUTGOING;

        // we have a duplicate entry. This means that the two nodes are trying to connect to each
        // other. In this case define the rule that the connection with the lowest address continues
        // the connection
        bool const should_replace =
            ((address_ < address) && is_outgoing) || ((address_ > address) && !is_outgoing);

        if (should_replace)
        {
          // cache the previous handle
          if (previous_handle != nullptr)
          {
            *previous_handle = it->second;
          }

          status = UpdateStatus::REPLACED;
        }
      }
    }
  }

  return status;
}

}  // namespace muddle
}  // namespace fetch
