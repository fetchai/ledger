#include "core/byte_array/encoders.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/router.hpp"
#include "network/muddle/muddle_register.hpp"
#include "network/muddle/dispatcher.hpp"

#include <memory>

namespace fetch {
namespace muddle {

namespace {

bool CompareAddress(uint8_t const *a, uint8_t const *b)
{
  bool equal = true;

  for (std::size_t i = 0; i < Packet::ADDRESS_SIZE; ++i)
  {
    if (a[i] != b[i])
    {
      equal = false;
      break;
    }
  }

  return equal;
}

bool operator==(Packet::RawAddress const &lhs, Packet::Address const &rhs)
{
  return CompareAddress(lhs.data(), rhs.pointer());
}

#if 0
bool operator==(Packet::Address const &lhs, Packet::RawAddress const &rhs)
{
  return CompareAddress(lhs.pointer(), rhs.data());
}
#endif

byte_array::ConstByteArray ToConstByteArray(Packet::RawAddress const &addr)
{
  byte_array::ByteArray buffer;
  buffer.Resize(addr.size());
  std::memcpy(buffer.pointer(), addr.data(), addr.size());
  return buffer;
}

/**
 * Generate a create a initial packet format
 *
 * @param from The address of the sender
 * @param service The service identifier
 * @param channel The channel / protocol identifier
 * @param counter The message number / counter
 * @param ttl  The value of the TTL
 * @param payload The reference to the payload to be send
 * @return A new packet with common field populated
 */
Router::PacketPtr FormatPacket(Packet::Address const &from,
                               uint16_t service,
                               uint16_t channel,
                               uint16_t counter,
                               uint8_t ttl,
                               Packet::Payload const &payload)
{
  auto packet = std::make_shared<Packet>(from);
  packet->SetService(service);
  packet->SetProtocol(channel);
  packet->SetMessageNum(counter);
  packet->SetTTL(ttl);
  packet->SetPayload(payload);

  return packet;
}

} // namespace

/**
 * Constructs a muddle router instance
 *
 * @param address The address of the current node
 * @param reg The connection register
 */
Router::Router(Router::Address const &address, MuddleRegister const &reg, Dispatcher &dispatcher)
  : address_(address)
  , register_(reg)
  , dispatcher_(dispatcher)
{
}

/**
 * Takes an input packet from the network layer and routes it across the network
 *
 * @param handle The handle of the receiving connection for the packet
 * @param packet The input packet to route
 */
void Router::Route(Handle handle, PacketPtr packet)
{
#if 1
  FETCH_LOG_INFO(LOGGING_NAME,
    "Routing packet: To: ", byte_array::ToBase64(packet->GetTarget()),
    " From: ", byte_array::ToBase64(packet->GetSender()),
    " Service: ", packet->GetService(), ':', packet->GetProtocol(),
    " Direct: ", packet->IsDirect(),
    " Bcast: ", packet->IsBroadcast(),
    " TTL: ", packet->GetTTL()
  );
#endif

  if (packet->IsDirect())
  {
    // when it is a direct message we must handle this
    DispatchDirect(handle, packet);
  }
  else if (packet->GetTargetRaw() == address_)
  {
    // when the message is targetted at use we must handle it
    DispatchPacket(packet);
  }
  else
  {
    // if this message does not belong to us we must route it along the path
    RoutePacket(packet);
  }
}

/**
 * Sends a payload directly to the connection specified from the handle.
 *
 * This function is intended to be used by internal objects of the muddle stack and not to be
 * exposed publically.
 *
 * @param handle The network handle identifying the target connection
 * @param service_num The service number for the payload
 * @param proto_num The protocol number for the payload
 * @param payload The payload contents
 */
void Router::SendDirect(Handle handle, uint16_t service_num, uint16_t proto_num, Payload const &payload)
{
  // format the packet
  auto pkt = std::make_shared<Packet>(address_);
  pkt->SetDirect(true);
  pkt->SetService(service_num);
  pkt->SetProtocol(proto_num);
  pkt->SetPayload(payload);

  SendToConnection(handle, pkt);
}

/**
 * Send an message to a target address
 *
 * @param address The address (public key) of the target machine
 * @param service The service identifier
 * @param channel The channel identifier
 * @param message The message to be sent
 */
void Router::Send(Address const &address, uint16_t service, uint16_t channel,
                  Payload const &message)
{
  // get the next counter for this message
  uint16_t const counter = dispatcher_.GetNextCounter();

  // format the packet
  auto packet = FormatPacket(address_, service, channel, counter, 10, message);
  packet->SetTarget(address);

  RoutePacket(packet, false);
}

/**
 * Send a message to a target address
 *
 * @param address The address (public key) of the target machine
 * @param service The service identifier
 * @param channel The channel identifier
 * @param message_num The message number of the request
 * @param payload The message payload to be sent
 */
void Router::Send(Address const &address, uint16_t service, uint16_t channel,
                  uint16_t message_num, Payload const &payload)
{
  // format the packet
  auto packet = FormatPacket(address_, service, channel, message_num, 10, payload);
  packet->SetTarget(address);

  RoutePacket(packet, false);
}

/**
 * Broadcast a message to all peers in the network
 *
 * @param service The service identifier
 * @param channel The channel identifier
 * @param message The message to be sent across the network
 */
void Router::Broadcast(uint16_t service, uint16_t channel, Payload const &payload)
{
  // get the next counter for this message
  uint16_t const counter = dispatcher_.GetNextCounter();

  auto packet = FormatPacket(address_, service, channel, counter, 10, payload);
  packet->SetBroadcast(true);

  RoutePacket(packet, false);
}

/**
 * Send a request and expect a response back from the target address
 *
 * @param request The request to be sent
 * @param service The service identifier
 * @param channel The channel identifier
 * @return The promise of a response back from the target address
 */
Router::Response Router::Exchange(Address const &address, uint16_t service, uint16_t channel,
                                  Payload const &request)
{
  // get the next counter for this message
  uint16_t const counter = dispatcher_.GetNextCounter();

  // register with the dispatcher that we are expecting a response
  auto promise = dispatcher_.RegisterExchange(service, channel, counter);

  // format the packet and route the packet
  auto packet = FormatPacket(address_, service, channel, counter, 10, request);
  packet->SetTarget(address);
  RoutePacket(packet, false);

  // return the response
  return Response(std::move(promise));
}

/**
 * Internal: Add an entry into the routing table for the given address and handle
 *
 * @param handle The handle to the connection
 * @param address The address associated with that handle
 */
void Router::AssociateHandleWithAddress(Handle handle, Packet::RawAddress const &address)
{
  {
    FETCH_LOCK(routing_table_lock_);

    // TODO(EJF): We are just blindly updating this datastructure at the moment might need some alternative
    // update logic in the future.
    routing_table_handles_[handle] = address;
    auto &metadata = routing_table_[address][handle];
    metadata.reliable = true; // TODO(EJF): This is always true at the moment but doesn't need to be
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Associating handle: ", handle, " with ", byte_array::ToBase64(ToConstByteArray(address)));
}

/**
 * Internal: Looks up the specified connection handle from a given address
 *
 * @param address The address to lookup the handle for.
 * @return The target handle for the connection, or zero on failure.
 */
Router::Handle Router::LookupHandle(Packet::RawAddress const &address) const
{
  Handle handle = 0;

  {
    FETCH_LOCK(routing_table_lock_);

    auto address_it = routing_table_.find(address);
    if (address_it != routing_table_.end())
    {
      auto const &handle_map = address_it->second;

      if (!handle_map.empty())
      {
        handle = handle_map.begin()->first;
      }
    }
  }

  return handle;
}

/**
 * Internal: Takes a given packet and sends it to the connection specified by the handle
 *
 * @param handle The handle to the network connection
 * @param packet The packet to be routed
 */
void Router::SendToConnection(Handle handle, PacketPtr packet)
{
  // internal method, we expect all inputs be valid at this stage
  assert(static_cast<bool>(packet));

  // lookup the connection
  auto conn = register_.LookupConnection(handle).lock();
  if (conn)
  {
    // serialize the packet to the buffer
    serializers::ByteArrayBuffer buffer;
    buffer << *packet;

    // dispatch to the connection object
    conn->Send(buffer.data());
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to route packet to handle: ", handle);
  }
}

/**
 * Attempt to route the packet to the require address(es)
 *
 * @param packet The packet to be routed
 * @param external Flag to signal that this packet originated from the network
 */
void Router::RoutePacket(PacketPtr packet, bool external)
{
  if (external)
  {
    // Handle TTL based routing timeout
    bool message_time_expired = true;
    if (packet->GetTTL() > 2u)
    {
      packet->SetTTL(static_cast<uint8_t>(packet->GetTTL() - 1u));
      message_time_expired = false;
    }

    if (message_time_expired)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Message has timed out");
      return;
    }
  }

  // broadcast packet
  if (packet->IsBroadcast())
  {
    // TODO(EJF): Implement replay cache
    FETCH_LOG_WARN(LOGGING_NAME, "!!! Unable to route broadcast packet yet need to implement replay cache");
  }
  else
  {
    // attempt to route to one of our direct peers
    Handle handle = LookupHandle(packet->GetTargetRaw());
    if (handle)
    {
      // one of our direct connections is the target address, route and complete
      SendToConnection(handle, packet);
      return;
    }

    // TODO(EJF): Implement kad-routing
    FETCH_LOG_WARN(LOGGING_NAME, "!!! Unable to route the packet currently");
  }
}

/**
 * Dispatch / Handle the direct packet from a single hop peer
 *
 * @param handle The handle to the originating connection
 * @param packet The packet that was received
 */
void Router::DispatchDirect(Handle handle, PacketPtr packet)
{
  FETCH_LOG_WARN(LOGGING_NAME, "==> Direct message sent to router");

  if (packet->GetService() == SERVICE_MUDDLE)
  {
    if ((PROTO_MUDDLE_ROUTING_REQUEST == packet->GetProtocol()) ||
        (PROTO_MUDDLE_ROUTING_REPLY == packet->GetProtocol()))
    {
      // make the association with
      AssociateHandleWithAddress(handle, packet->GetSenderRaw());

      // send back a direct response if that is required
      if (PROTO_MUDDLE_ROUTING_REQUEST == packet->GetProtocol())
      {
        SendDirect(handle, SERVICE_MUDDLE, PROTO_MUDDLE_ROUTING_REPLY, Packet::Payload{});
      }
    }
  }
}

/**
 * Dispatch / Handle a normally (routed) packet
 *
 * @param packet The packet that was received
 */
void Router::DispatchPacket(PacketPtr packet)
{
  FETCH_LOG_WARN(LOGGING_NAME, "==> Message routed to this node");

  // determine if this was an exchange based node
  if (dispatcher_.Dispatch(packet))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "==> Succesfully dispatched message to pending promise");

    // the dispatcher has "claimed" this packet as there was an outstanding promise waiting for it
    return;
  }

  // If no exchange message has claimed this then attempt to dispatch it through our normal system
  // of message subscriptions.
  if (registrar_.Dispatch(packet))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "==> Succesfully dispatched message to subscription");
    return;
  }

  // TODO(EJF): Implement simple dispatch routines
  FETCH_LOG_WARN(LOGGING_NAME, "Routed packet has no home, how sad");
}

/**
 * Subscribes to messages from network with a given service and channel
 *
 * @param service The identifier for the service
 * @param channel The identifier for the channel
 * @return A valid pointer if the successful, otherwise an invalid pointer
 */
MuddleEndpoint::SubscriptionPtr Router::Subscribe(uint16_t service, uint16_t channel)
{
  return registrar_.Register(service, channel);
}

/**
 * Subscribes to messages from network with a given service and channel
 *
 * @param address The reference to the address
 * @param service The identifier for the service
 * @param channel The identifier for the channel
 * @return A valid pointer if the successful, otherwise an invalid pointer
 */
MuddleEndpoint::SubscriptionPtr Router::Subscribe(Address const &address, uint16_t service, uint16_t channel)
{
  return registrar_.Register(address, service, channel);
}

} // namespace p2p
} // namespace fetch
