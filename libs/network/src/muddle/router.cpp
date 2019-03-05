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

#include "network/muddle/router.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/service_ids.hpp"
#include "crypto/fnv.hpp"
#include "network/muddle/dispatcher.hpp"
#include "network/muddle/muddle_register.hpp"
#include "network/muddle/packet.hpp"

#include <cstring>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <utility>

static constexpr uint8_t DEFAULT_TTL = 40;

using fetch::byte_array::ToBase64;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace muddle {

namespace {

/**
 * Generate an id for echo cancellation id
 *
 * @param packet The input packet to generate the echo id
 * @return
 */
std::size_t GenerateEchoId(Packet const &packet)
{
  crypto::FNV hash;
  hash.Reset();

  auto const service = packet.GetService();
  auto const channel = packet.GetProtocol();
  auto const counter = packet.GetMessageNum();

  hash.Update(packet.GetSenderRaw().data(), packet.GetSenderRaw().size());
  hash.Update(reinterpret_cast<uint8_t const *>(&service), sizeof(service));
  hash.Update(reinterpret_cast<uint8_t const *>(&channel), sizeof(channel));
  hash.Update(reinterpret_cast<uint8_t const *>(&counter), sizeof(counter));

  return hash.Final<std::size_t>();
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
  return std::memcmp(a, b, Packet::ADDRESS_SIZE * sizeof(uint8_t)) == 0;
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
Router::PacketPtr FormatDirect(Packet::Address const &from, NetworkId const &network,
                               uint16_t service, uint16_t channel)
{
  auto packet = std::make_shared<Packet>(from, network.value());
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
Router::PacketPtr FormatPacket(Packet::Address const &from, NetworkId const &network,
                               uint16_t service, uint16_t channel, uint16_t counter, uint8_t ttl,
                               Packet::Payload const &payload)
{
  auto packet = std::make_shared<Packet>(from, network.value());
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

  oss << "To: " << ToBase64(packet.GetTarget()) << " From: " << ToBase64(packet.GetSender())
      << " Route: " << NetworkId{packet.GetNetworkId()}.ToString() << ':' << packet.GetService()
      << ':' << packet.GetProtocol() << ':' << packet.GetMessageNum()
      << " Type: " << (packet.IsDirect() ? 'D' : 'R') << (packet.IsBroadcast() ? 'B' : 'T')
      << (packet.IsExchange() ? 'X' : 'F') << " TTL: " << static_cast<std::size_t>(packet.GetTTL());

  return oss.str();
}

}  // namespace

/**
 * Convert one address format to another
 *
 * @param address The input address
 * @return The output address
 */
Packet::RawAddress Router::ConvertAddress(Packet::Address const &address)
{
  Packet::RawAddress raw_address;

  if (raw_address.size() != address.size())
  {
    throw std::runtime_error(
        "Unable to convert one address to another: raw:" + std::to_string(raw_address.size()) +
        ", actual: " + std::to_string(address.size()));
  }

  for (std::size_t i = 0; i < address.size(); ++i)
  {
    raw_address[i] = address[i];
  }

  return raw_address;
}

Packet::Address Router::ConvertAddress(Packet::RawAddress const &address)
{
  return {address.data(), address.size()};
}

/**
 * Constructs a muddle router instance
 *
 * @param address The address of the current node
 * @param reg The connection register
 */
Router::Router(NetworkId network_id, Address address, MuddleRegister const &reg,
               Dispatcher &dispatcher, Prover *prover)
  : address_(std::move(address))
  , address_raw_(ConvertAddress(address_))
  , register_(reg)
  , dispatcher_(dispatcher)
  , network_id_(std::move(network_id))
  , prover_(prover)
  , dispatch_thread_pool_(network::MakeThreadPool(NUMBER_OF_ROUTER_THREADS, "Router"))
{}

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

inline bool Router::Genuine(PacketPtr const &p) const
{
  return p->Verify() || !(prover_ || p->IsStamped());
}

Router::PacketPtr const &Router::Sign(PacketPtr const &p) const
{
  if (prover_)
  {
    p->Sign(*prover_);
  }
  return p;
}

/**
 * Takes an input packet from the network layer and routes it across the network
 *
 * @param handle The handle of the receiving connection for the packet
 * @param packet The input packet to route
 */
void Router::Route(Handle handle, PacketPtr packet)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Routing packet: ", DescribePacket(*packet));

  // discard all foreign packets
  if (packet->GetNetworkId() != network_id_.value())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Discarding foreign packet: ", DescribePacket(*packet), " at ",
                   ToBase64(address_), ":", network_id_.ToString());
    return;
  }

  if (packet->IsDirect())
  {
    // when it is a direct message we must handle this
    if (Genuine(packet))
    {
      DispatchDirect(handle, packet);
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Packet's authenticity not verified:", DescribePacket(*packet));
    }
  }
  else if (packet->GetTargetRaw() == address_)
  {
    // when the message is targetted at us we must handle it
    if (Genuine(packet))
    {
      Address transmitter;

      if (HandleToDirectAddress(handle, transmitter))
      {
        DispatchPacket(packet, transmitter);
      }
      else
      {
        // The connection has gone away while we were processing things so far.
        FETCH_LOG_WARN(LOGGING_NAME,
                       "Cannot get transmitter address for packet: ", DescribePacket(*packet));
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Packet's authenticity not verified:", DescribePacket(*packet));
    }
  }
  else
  {
    // update the routing table if required
    // TODO(KLL): this may not be the association we're looking for.
    AssociateHandleWithAddress(handle, packet->GetSenderRaw(), false);

    // if this message does not belong to us we must route it along the path
    RoutePacket(packet);
  }
}

/**
 * tells the router that it should add a connection. In practise this means that it should
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
  auto packet = FormatDirect(address_, network_id_, SERVICE_MUDDLE, CHANNEL_ROUTING);
  packet->SetExchange(true);  // signal that this is the request half of a direct message
  Sign(packet);

  // send it
  SendToConnection(handle, packet);
}

void Router::RemoveConnection(Handle /*handle*/)
{
  // TODO(EJF): Need to tear down handle routes etc. Also in more complicated scenario implement
  //            alternative routing
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
  auto packet =
      FormatPacket(address_, network_id_, service, channel, counter, DEFAULT_TTL, message);
  packet->SetTarget(address);
  Sign(packet);

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
void Router::Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
                  Payload const &payload)
{
  // format the packet
  auto packet =
      FormatPacket(address_, network_id_, service, channel, message_num, DEFAULT_TTL, payload);
  packet->SetTarget(address);
  Sign(packet);

  FETCH_LOG_DEBUG(LOGGING_NAME, "Exchange Response: ", ToBase64(address), " (", service, '-',
                  channel, '-', message_num, ")");

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

  auto packet =
      FormatPacket(address_, network_id_, service, channel, counter, DEFAULT_TTL, payload);
  packet->SetBroadcast(true);
  Sign(packet);

  RoutePacket(packet, false);
}

/**
 * Get a copy of the current routing table
 *
 * @return List of IDs from the routing table.
 */
Router::RoutingTable Router::GetRoutingTable() const
{
  FETCH_LOCK(routing_table_lock_);
  return routing_table_;
}

/**
 * Lookup a routing
 *
 * @return The address corresponding to a handle in the table.
 */
bool Router::HandleToDirectAddress(const Router::Handle &handle, Router::Address &address) const
{
  FETCH_LOCK(routing_table_lock_);
  auto address_it = direct_address_map_.find(handle);
  if (address_it != direct_address_map_.end())
  {
    address = address_it->second;
    return true;
  }
  return false;
}

/**
 * Helper function which prints out the content of the routing table and the direct_addres_map
 * @param prefix The prefix which will be used in the logging.
 */
void Router::Debug(std::string const &prefix) const
{
  FETCH_LOCK(routing_table_lock_);
  FETCH_LOG_WARN(LOGGING_NAME, prefix,
                 "direct_address_map_: --------------------------------------");
  for (const auto &routing : direct_address_map_)
  {
    auto output = ToBase64(routing.second);
    FETCH_LOG_WARN(LOGGING_NAME, prefix, static_cast<std::string>(output),
                   " -> handle=", std::to_string(routing.first), " direct=by definition");
  }
  FETCH_LOG_WARN(LOGGING_NAME, prefix,
                 "direct_address_map_: --------------------------------------");

  FETCH_LOG_WARN(LOGGING_NAME, prefix, "routing_table_: --------------------------------------");
  for (const auto &routing : routing_table_)
  {
    ByteArray output(routing.first.size());
    std::copy(routing.first.begin(), routing.first.end(), output.pointer());
    FETCH_LOG_WARN(LOGGING_NAME, prefix, static_cast<std::string>(ToBase64(output)),
                   " -> handle=", std::to_string(routing.second.handle),
                   " direct=", routing.second.direct);
  }
  FETCH_LOG_WARN(LOGGING_NAME, prefix, "routing_table_: --------------------------------------");
  registrar_.Debug(prefix);
}

/**
 * Periodic call initiated from the main muddle instance used for periodic maintenance of the
 * router
 */
void Router::Cleanup()
{
  CleanEchoCache();
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
  auto promise = dispatcher_.RegisterExchange(service, channel, counter, address);

  FETCH_LOG_DEBUG(LOGGING_NAME, "Exchange Request: ", ToBase64(address), " (", service, '-',
                  channel, '-', counter, ") prom: ", promise->id());

  // format the packet and route the packet
  auto packet =
      FormatPacket(address_, network_id_, service, channel, counter, DEFAULT_TTL, request);
  packet->SetTarget(address);
  packet->SetExchange();
  Sign(packet);

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
MuddleEndpoint::SubscriptionPtr Router::Subscribe(Address const &address, uint16_t service,
                                                  uint16_t channel)
{
  return registrar_.Register(address, service, channel);
}

MuddleEndpoint::AddressList Router::GetDirectlyConnectedPeers() const
{
  AddressList addresses{};

  FETCH_LOCK(routing_table_lock_);
  for (auto const &entry : routing_table_)
  {
    if (entry.second.direct)
    {
      // lookup the connection
      auto connection = register_.LookupConnection(entry.second.handle).lock();

      if (connection && connection->is_alive())
      {
        addresses.emplace_back(ConvertAddress(entry.first));
      }
    }
  }

  return addresses;
}

/**
 * Checks if there is an active connection with the given address.
 * @param target The address of the peer we want to check
 * @return true if there is an active connection
 */
bool Router::IsConnected(Address const &target) const
{
  auto raw_address = ConvertAddress(target);
  auto iter        = routing_table_.find(raw_address);
  bool connected   = false;
  if (iter != routing_table_.end())
  {
    auto conn = register_.LookupConnection(iter->second.handle).lock();
    if (conn)
    {
      connected = conn->is_alive();
    }
  }
  return connected;
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
bool Router::AssociateHandleWithAddress(Handle handle, Packet::RawAddress const &address,
                                        bool direct)
{
  bool update_complete = false;

  // At the moment these updates (and by extension the routing logic) works on a first
  // come first served basis. This is not reliable longer term and will need tweaking

  // sanity check
  assert(handle);

  // never allow the current node address to be added to the routing table
  if (address != address_raw_)
  {
    FETCH_LOCK(routing_table_lock_);

    // lookup (or create) the routing table entry
    auto &routing_data = routing_table_[address];

    bool const is_empty = (routing_data.handle == 0);

    // an update is only valid when the connection is direct.
    bool const is_different = (routing_data.handle != handle) || (routing_data.direct != direct);
    bool const is_update    = (routing_data.handle && direct && is_different);

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

      if (direct)
      {
        direct_address_map_[handle] = ToConstByteArray(address);
        if (prev_handle)
        {
          direct_address_map_.erase(prev_handle);
        }
      }
    }
  }

  if (update_complete)
  {
    char const *route_type = (direct) ? "direct" : "normal";

    FETCH_LOG_INFO(LOGGING_NAME, "Adding ", route_type,
                   " route for: ", ToBase64(ToConstByteArray(address)));
  }

  return update_complete;
}

/**
 * Internal: Looks up the specified connection handle from a given address
 *
 * @param address The address to lookup the handle for.
 * @return The target handle for the connection, or zero on failure.
 */
Router::Handle Router::LookupHandleFromAddress(Packet::Address const &address) const
{
  auto raddr = ConvertAddress(address);
  return LookupHandle(raddr);
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
 * Looks up a random handle from the routing table.
 * @param address paremeter not used
 * @return The random handle, or zero if the routing table is empty
 */
Router::Handle Router::LookupRandomHandle(Packet::RawAddress const & /*address*/) const
{
  Handle handle = 0;

  std::random_device rd;
  std::mt19937       rng(rd());

  {
    FETCH_LOCK(routing_table_lock_);

    if (!routing_table_.empty())
    {
      // decide the random index to access
      std::size_t const element = rng() % routing_table_.size();

      // advance the iterator to the correct offset
      auto it = routing_table_.cbegin();
      std::advance(it, static_cast<std::ptrdiff_t>(element));

      // update the handle
      handle = it->second.handle;
    }
  }

  return handle;
}

/**
 * Kills active connection by closing the socket and removing the peer from the data structures.
 * @param handle The connection handle we want to remove
 * @param peer The address of the peer
 */
void Router::KillConnection(Handle handle, Address const &peer)
{
  auto conn = register_.LookupConnection(handle).lock();
  if (conn)
  {
    FETCH_LOCK(routing_table_lock_);
    conn->Close();
    routing_table_.erase(ConvertAddress(peer));
    direct_address_map_.erase(handle);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "No connection object available to KillConnection(", handle, ")");
  }
}

/**
 * Kills active connection by closing the socket and removing the peer from the data structures.
 * @param handle The connection handle we want to remove
 */
void Router::KillConnection(Handle handle)
{
  Address address;
  {
    FETCH_LOCK(routing_table_lock_);
    auto it = direct_address_map_.find(handle);
    if (it != direct_address_map_.end())
    {
      address = it->second;
    }
  }
  if (address.size() > 0)
  {
    KillConnection(handle, address);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Address not found for handle ", handle);
  }
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
      dispatcher_.NotifyMessage(handle, packet->GetService(), packet->GetProtocol(),
                                packet->GetMessageNum());
    }

    // serialize the packet to the buffer
    serializers::ByteArrayBuffer buffer;
    buffer << *packet;

    FETCH_LOG_WARN(LOGGING_NAME, "Sending out", DescribePacket(*packet));
    // dispatch to the connection object
    conn->Send(buffer.data());
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to route packet to handle: ", handle);
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
    {
      return;
    }
  }

  /// Step 2. Route and dispatch the packet

  // broadcast packet
  if (packet->IsBroadcast())
  {
    if (packet->GetSender() != address_)
    {
      DispatchPacket(packet, address_);
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

    // if direct routing fails then randomly select a handle. In future a better routing scheme
    // should be implemented.
    handle = LookupRandomHandle(packet->GetTargetRaw());
    if (handle)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Speculative routing to peer: ", ToBase64(packet->GetTarget()));
      SendToConnection(handle, packet);
    }
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
      if (blacklist_.Contains(packet->GetSenderRaw()))
      {
        // this is where we prevent incoming connections.
        FETCH_LOG_WARN(LOGGING_NAME, "Blacklisting:", ToBase64(packet->GetSender()),
                       "  killing handle=", handle);
        KillConnection(handle);
        return;
      }

      // make the association with
      AssociateHandleWithAddress(handle, packet->GetSenderRaw(), true);

      // send back a direct response if that is required
      if (packet->IsExchange())
      {
        SendToConnection(
            handle, Sign(FormatDirect(address_, network_id_, SERVICE_MUDDLE, CHANNEL_ROUTING)));
      }
    }
  }
}

/**
 * Dispatch / Handle a normally (routed) packet
 *
 * @param packet The packet that was received
 */
void Router::DispatchPacket(PacketPtr packet, Address transmitter)
{
  dispatch_thread_pool_->Post([this, packet, transmitter]() {
    bool const isPossibleExchangeResponse = !packet->IsExchange();

    // determine if this was an exchange based node
    if (isPossibleExchangeResponse && dispatcher_.Dispatch(packet))
    {
      // the dispatcher has "claimed" this packet as there was an outstanding promise waiting for it
      return;
    }

    // If no exchange message has claimed this then attempt to dispatch it through our normal system
    // of message subscriptions.
    if (registrar_.Dispatch(packet, transmitter))
    {
      return;
    }

    FETCH_LOG_WARN(LOGGING_NAME,
                   "Unable to locate handler for routed message. Net: ", packet->GetNetworkId(),
                   " Service: ", packet->GetService(), " Channel: ", packet->GetProtocol());
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
  std::size_t const index = GenerateEchoId(packet);

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

/**
 * Periodic function used to trim the echo cache
 */
void Router::CleanEchoCache()
{
  FETCH_LOCK(echo_cache_lock_);

  auto const now = Clock::now();

  auto it = echo_cache_.begin();
  while (it != echo_cache_.end())
  {
    // calculate the time delta
    auto const delta = now - it->second;

    if (delta > std::chrono::seconds{30})
    {
      // remove the element
      it = echo_cache_.erase(it);
    }
    else
    {
      // move on to the next element in the cache
      ++it;
    }
  }
}

void Router::Blacklist(Address const &target)
{
  blacklist_.Add(target);
}

void Router::Whitelist(Address const &target)
{
  blacklist_.Remove(target);
}

bool Router::IsBlacklisted(Address const &target) const
{
  return blacklist_.Contains(target);
}

void Router::DropPeer(Address const &peer)
{
  FETCH_LOG_WARN(LOGGING_NAME, "Dropping peer from router: ", ToBase64(peer));
  Handle h = LookupHandle(ConvertAddress(peer));
  if (h)
  {
    KillConnection(h, peer);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Not dropping ", ToBase64(peer), " -- not connected");
  }
}

void Router::DropHandle(Router::Handle handle, Address const &peer)
{
  if (handle)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Dropping peer from router: ", ToBase64(peer));
    KillConnection(handle, peer);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "DropHandle got invalid handle: ", handle);
  }
}

}  // namespace muddle
}  // namespace fetch
