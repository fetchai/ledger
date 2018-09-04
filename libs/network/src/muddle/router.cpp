#include "core/byte_array/encoders.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/router.hpp"
#include "network/muddle/muddle_register.hpp"
#include "network/muddle/dispatcher.hpp"

#include <memory>
#include <sstream>
#include <core/service_ids.hpp>

static constexpr uint8_t DEFAULT_TTL = 40;

using fetch::byte_array::ToBase64;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace muddle {

namespace {

/**
 * Combine service, channel and counter into a single incde
 *
 * @param service The service id
 * @param channel The channel id
 * @param counter The message counter id
 * @return The aggregated counter
 */
uint64_t Combine(uint16_t service, uint16_t channel, uint16_t counter)
{
  uint64_t id = 0;

  id |= static_cast<uint64_t>(service) << 32u;
  id |= static_cast<uint64_t>(channel) << 16u;
  id |= static_cast<uint64_t>(counter);

  return id;
}

/**
 * Internal; Function used to compare two fixed size addresses
 *
 * @param a The pointer to the start of one of the addresses
 * @param b The pointer to the start of the other address
 * @return true if the two sequences are the same length, otherwise false
 */
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

/**
 * Comparison operation
 *
 * @param lhs Reference initial address to compare
 * @param rhs Reference to the other address to compare
 * @return true if the addresses are the same, otherwise false
 */
bool operator==(Packet::RawAddress const &lhs, Packet::Address const &rhs)
{
  return CompareAddress(lhs.data(), rhs.pointer());
}

/**
 * Convert a raw address into a byte array
 *
 * @param addr The reference to the address to convert
 * @return The converted (output) byte array
 */
ConstByteArray ToConstByteArray(Packet::RawAddress const &addr)
{
  ByteArray buffer;
  buffer.Resize(addr.size());
  std::memcpy(buffer.pointer(), addr.data(), addr.size());
  return buffer;
}

/**
 * Generate a direct packet
 *
 * @param from The address of the sender
 * @param service The service identifier
 * @param channel The channel identifier
 * @return The packet
 */
Router::PacketPtr FormatDirect(Packet::Address const &from, uint16_t service, uint16_t channel)
{
  auto packet = std::make_shared<Packet>(from);
  packet->SetService(service);
  packet->SetProtocol(channel);
  packet->SetDirect(true);

  return packet;
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

std::string DescribePacket(Packet const &packet)
{
  std::ostringstream oss;

  oss << "To: " << ToBase64(packet.GetTarget())
      << " From: " << ToBase64(packet.GetSender())
      << " Route: " << packet.GetService() << ':' << packet.GetProtocol() << ':' << packet.GetMessageNum()
      << " Type: "
      << (packet.IsDirect() ? 'D' : 'R')
      << (packet.IsBroadcast() ? 'B' : 'T')
      << (packet.IsExchange() ? 'X' : 'F')
      << " TTL: " << static_cast<std::size_t>(packet.GetTTL());

  return oss.str();
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
  , dispatch_thread_pool_(network::MakeThreadPool(1))
{
}

/**
 * Starts the routers internal dispatch thread pool
 */
void Router::Start()
{
  dispatch_thread_pool_->Start();
}

/**
 * Stops the routers internal dispatch thread pool
 */
void Router::Stop()
{
  dispatch_thread_pool_->Stop();
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
  FETCH_LOG_DEBUG(LOGGING_NAME, "Routing packet: ", DescribePacket(*packet));
#endif

  // update the routing table if required
  AssociateHandleWithAddress(handle, packet->GetSenderRaw(), false);

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
 * Tells the router that it should add a connection. In practise this means that it should
 * begin identity exchange with the node and update the routing table accordingly.
 *
 * Before calling this function, the handle must be registered with the connection register. If
 * it isn't then the router will be unable to lookup the pointer to the `AbstractConnection` and
 * will not send the message.
 *
 * @param handle The handle to the connection
 */
void Router::AddConnection(Handle handle)
{
  // create and format the packet
  auto packet = FormatDirect(address_, SERVICE_MUDDLE, CHANNEL_ROUTING);
  packet->SetExchange(true); // signal that this is the request half of a direct message

  // send the
  SendToConnection(handle, packet);
}

void Router::RemoveConnection(Handle handle)
{
  // TODO(EJF): Need to tear down handle routes etc. Also in more complicated scenario implement alternative routing
}

#if 0
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
#endif

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
  auto packet = FormatPacket(address_, service, channel, counter, DEFAULT_TTL, message);
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
  auto packet = FormatPacket(address_, service, channel, message_num, DEFAULT_TTL, payload);
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

  auto packet = FormatPacket(address_, service, channel, counter, DEFAULT_TTL, payload);
  packet->SetBroadcast(true);

  RoutePacket(packet, false);
}

  /**
 * Get all the IDs we're currently connected to.
 *
 * @return List of IDs from the routing table.
 */
std::list<std::pair<Packet::Address, Router::Handle>> Router::GetPeerIdentities()
{
  using RET_PAIR = std::pair<Packet::Address, Router::Handle>;

  std::list<RET_PAIR> res;
  FETCH_LOCK(routing_table_lock_);
  for(auto &val : routing_table_)
  {
    Packet::RawAddress raw = val.first;

    byte_array::ByteArray cooked;
    cooked.Resize(std::size_t{Packet::ADDRESS_SIZE});
    std::memcpy(cooked.pointer(), raw.data(), Packet::ADDRESS_SIZE);

    RET_PAIR new_res = RET_PAIR(cooked, val.second.handle);
    res.push_back(new_res);
  }
  return res;
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
  auto packet = FormatPacket(address_, service, channel, counter, DEFAULT_TTL, request);
  packet->SetTarget(address);
  packet->SetExchange();
  RoutePacket(packet, false);

  // return the response
  return Response(std::move(promise));
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

/**
 * Internal: Add an entry into the routing table for the given address and handle.
 *
 * This call might override an existing routing table entry in the case that the connection
 * becomes direct, or is updated to an alternate direct connection.
 *
 * @param handle The handle to the connection
 * @param address The address associated with that handle
 * @param direct Signal that this connection is a direct connection
 * @return true if an update was performed, otherwise false
 */
bool Router::AssociateHandleWithAddress(Handle handle, Packet::RawAddress const &address, bool direct)
{
  bool update_complete = false;

  // TODO(EJF): At the moment these updates (and by extension the routing logic) works on a first come
  // first served basis. This is not reliable longer term and will need tweaking

  // sanity check
  assert(handle);

  {
    FETCH_LOCK(routing_table_lock_);

    // lookup (or create) the routing table entry
    auto &routing_data = routing_table_[address];

    bool const is_empty = (routing_data.handle == 0);

    // an update is only valid when the connection is direct.
    bool const is_different = (routing_data.handle != handle) || (routing_data.direct != direct);
    bool const is_update = (routing_data.handle && direct && is_different);

    // update the routing table if required
    if (is_empty || is_update)
    {
      // replacing an existing entry
      Handle prev_handle = routing_data.handle;

      // update the table
      routing_data.handle = handle;
      routing_data.direct = direct;

      // remove association of the previous handle with the address (if required)
      if (prev_handle)
      {
        routing_table_handles_[prev_handle].erase(address);
      }

      // associate the new handle with the address
      routing_table_handles_[handle].insert(address);

      // signal an update was made to the table
      update_complete = true;
    }
  }

  if (update_complete)
  {
    char const *route_type = (direct) ? "direct" : "normal";

    FETCH_LOG_INFO(LOGGING_NAME, "==> Adding ", route_type, " route for: ", ToBase64(ToConstByteArray(address)));
  }

  return update_complete;
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
      auto const &routing_data = address_it->second;

      handle = routing_data.handle;
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
    // determine if this packet originated from this node and that we are expecting an exchange
    if (packet->IsExchange() && (address_ == packet->GetSender()))
    {
      // notify the dispatcher about the message so that it can associate the connection handle
      // with any pending promises. This is required to ensure clean handling of promises which
      // fail due to connection loss.
      dispatcher_.NotifyMessage(handle, packet->GetService(),
                                packet->GetProtocol(), packet->GetMessageNum());
    }

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
  /// Step 1. Determine if we should drop this packet (for whatever reason)
  if (external)
  {
    // Handle TTL based routing timeout
    bool message_time_expired = true;
    if (packet->GetTTL() > 2u)
    {
      // decrement the TTL
      packet->SetTTL(static_cast<uint8_t>(packet->GetTTL() - 1u));
      message_time_expired = false;
    }

    if (message_time_expired)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Message has timed out (TTL): ", DescribePacket(*packet));
      return;
    }

    // if this packet is a broadcast echo we should no longer route this packet
    if (packet->IsBroadcast() && IsEcho(*packet))
      return;
  }

  /// Step 2. Route and dispatch the packet

  // broadcast packet
  if (packet->IsBroadcast())
  {
    if (packet->GetSender() != address_)
    {
      DispatchPacket(packet);
    }

    // serialize the packet to the buffer
    serializers::ByteArrayBuffer buffer;
    buffer << *packet;

    // broadcast the data across the network
    register_.Broadcast(buffer.data());
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
  FETCH_LOG_DEBUG(LOGGING_NAME, "==> Direct message sent to router");

  if (SERVICE_MUDDLE == packet->GetService())
  {
    if (CHANNEL_ROUTING == packet->GetProtocol())
    {
      // make the association with
      AssociateHandleWithAddress(handle, packet->GetSenderRaw(), true);

      // send back a direct response if that is required
      if (packet->IsExchange())
      {
        SendToConnection(handle, FormatDirect(address_, SERVICE_MUDDLE, CHANNEL_ROUTING));
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
  FETCH_LOG_DEBUG(LOGGING_NAME, "==> Message routed to this node");

  dispatch_thread_pool_->Post([this, packet]() {

    // determine if this was an exchange based node
    if (dispatcher_.Dispatch(packet))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "==> Succesfully dispatched message to pending promise");

      // the dispatcher has "claimed" this packet as there was an outstanding promise waiting for it
      return;
    }

    // If no exchange message has claimed this then attempt to dispatch it through our normal system
    // of message subscriptions.
    if (registrar_.Dispatch(packet))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "==> Succesfully dispatched message to subscription");
      return;
    }

    // TODO(EJF): Implement simple dispatch routines
    FETCH_LOG_WARN(LOGGING_NAME, "Routed packet has no home, how sad");
  });
}

/**
 * Check to see if the packet packet is an echo
 *
 * @param packet The reference to the packet
 * @param register_echo Signal if the echo should be registered (if not already in cache)
 * @return true if the packet is an echo, otherwise false
 */
bool Router::IsEcho(Packet const &packet, bool register_echo)
{
  bool is_echo = true;

  // combine the 3 fields together into a single index
  uint64_t const index = Combine(packet.GetService(), packet.GetProtocol(), packet.GetMessageNum());

  {
    FETCH_LOCK(echo_cache_lock_);

    // lookup if the echo is in the cache
    auto it = echo_cache_.find(index);
    if (it == echo_cache_.end())
    {
      // register the echo (in needed)
      if (register_echo)
      {
        echo_cache_[index] = Clock::now();
      }

      is_echo = false;
    }
  }

  return is_echo;
}

} // namespace p2p
} // namespace fetch
