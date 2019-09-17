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

#include "dispatcher.hpp"
#include "muddle_logging_name.hpp"
#include "muddle_register.hpp"
#include "router.hpp"
#include "routing_message.hpp"
#include "xor_metric.hpp"

#include "core/byte_array/encoders.hpp"
#include "core/containers/set_intersection.hpp"
#include "core/logging.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/fnv.hpp"
#include "muddle/packet.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

static constexpr uint8_t DEFAULT_TTL = 40;

using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ToBase64;

namespace fetch {
namespace muddle {
namespace {

constexpr char const *BASE_NAME = "Router";

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
  auto const channel = packet.GetChannel();
  auto const counter = packet.GetMessageNum();

  hash.Update(packet.GetSenderRaw().data(), packet.GetSenderRaw().size());
  hash.Update(reinterpret_cast<uint8_t const *>(&service), sizeof(service));
  hash.Update(reinterpret_cast<uint8_t const *>(&channel), sizeof(channel));
  hash.Update(reinterpret_cast<uint8_t const *>(&counter), sizeof(counter));

  std::size_t out = 0;

  static_assert(sizeof(out) == hash.size_in_bytes,
                "Output type has incorrect size to contain hash");
  hash.Final(reinterpret_cast<uint8_t *>(&out));

  return out;
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
  return {std::move(buffer)};
}

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
  packet->SetChannel(channel);
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
      << ':' << packet.GetChannel() << ':' << packet.GetMessageNum()
      << " Type: " << (packet.IsDirect() ? 'D' : 'R') << (packet.IsBroadcast() ? 'B' : 'U')
      << (packet.IsExchange() ? 'X' : 'N') << " TTL: " << static_cast<std::size_t>(packet.GetTTL());

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
Router::Router(NetworkId network_id, Address address, MuddleRegister &reg, Dispatcher &dispatcher,
               Prover *prover, bool sign_broadcasts)
  : name_{GenerateLoggingName(BASE_NAME, network_id)}
  , address_(std::move(address))
  , address_raw_(ConvertAddress(address_))
  , register_(reg)
  , dispatcher_(dispatcher)
  , registrar_(network_id)
  , network_id_(network_id)
  , prover_(prover)
  , sign_broadcasts_(prover && sign_broadcasts)
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

bool Router::Genuine(PacketPtr const &p) const
{
  if (p->IsBroadcast())
  {
    // broadcasts are only verified if really needed
    return !sign_broadcasts_ || p->Verify();
  }
  if (p->IsStamped())
  {
    // stamped packages are verified in any circumstances
    return p->Verify();
  }
  // non-stamped packages are genuine in a trusted network
  return !prover_;
}

Router::PacketPtr const &Router::Sign(PacketPtr const &p) const
{
  if (prover_ && (sign_broadcasts_ || !p->IsBroadcast()))
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
  FETCH_LOG_TRACE(logging_name_, "RX: (conn: ", handle, ") ", DescribePacket(*packet));

  // discard all foreign packets
  if (packet->GetNetworkId() != network_id_.value())
  {
    FETCH_LOG_WARN(logging_name_, "Discarding foreign packet: ", DescribePacket(*packet), " at ",
                   ToBase64(address_), ":", network_id_.ToString());
    return;
  }

  if (!Genuine(packet))
  {
    FETCH_LOG_WARN(logging_name_, "Packet's authenticity not verified:", DescribePacket(*packet));
    return;
  }

  if (packet->IsDirect())
  {
    // when it is a direct message we must handle this
    DispatchDirect(handle, packet);
  }
  else if (packet->GetTargetRaw() == address_)
  {
    // we do not care about the transmitter, since this was an addition for the trust system.
    DispatchPacket(packet, packet->GetSender());
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

void Router::ConnectionDropped(Handle handle)
{
  FETCH_LOG_INFO(logging_name_, "Connection ", handle, " dropped");

  FETCH_LOCK(routing_table_lock_);
  for (auto it = routing_table_.begin(); it != routing_table_.end();)
  {
    if (it->second.handle == handle)
    {
      it = routing_table_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

Address const &Router::GetAddress() const
{
  return address_;
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

  Send(address, service, channel, counter, message, OPTION_DEFAULT);
}

void Router::Send(Address const &address, uint16_t service, uint16_t channel,
                  Payload const &message, Options options)
{
  uint16_t const counter = dispatcher_.GetNextCounter();

  Send(address, service, channel, counter, message, options);
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
  Send(address, service, channel, message_num, payload, OPTION_DEFAULT);
}

void Router::Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
                  Payload const &payload, Options options)
{
  // format the packet
  auto packet =
      FormatPacket(address_, network_id_, service, channel, message_num, DEFAULT_TTL, payload);
  packet->SetTarget(address);

  if (options & OPTION_EXCHANGE)
  {
    packet->SetExchange(true);
  }

  Sign(packet);

  FETCH_LOG_TRACE(logging_name_, "Exchange Response: ", ToBase64(address), " (", service, '-',
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

  FETCH_LOG_TRACE(logging_name_, "Exchange Request: ", ToBase64(address), " (", service, '-',
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
  auto peers = GetDirectlyConnectedPeerSet();
  return {std::make_move_iterator(peers.begin()), std::make_move_iterator(peers.end())};
}

Router::AddressSet Router::GetDirectlyConnectedPeerSet() const
{
  AddressSet peers{};
  auto const current_direct_peers = register_.GetCurrentAddressSet();

  FETCH_LOCK(routing_table_lock_);
  for (auto const &address : current_direct_peers)
  {
    auto const raw_address = ConvertAddress(address);
    if (routing_table_.find(raw_address) != routing_table_.end())
    {
      peers.emplace(address);
    }
  }

  return peers;
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
 * @return The status corresponding to the update
 */
Router::UpdateStatus Router::AssociateHandleWithAddress(Handle                    handle,
                                                        Packet::RawAddress const &address,
                                                        bool                      direct)
{
  UpdateStatus status{UpdateStatus::NO_CHANGE};

  // At the moment these updates (and by extension the routing logic) works on a first
  // come first served basis. This is not reliable longer term and will need tweaking

  // sanity check
  assert(handle);

  bool display{false};

  // never allow the current node address to be added to the routing table
  if (address != address_raw_)
  {
    FETCH_LOCK(routing_table_lock_);

    // lookup (or create) the routing table entry
    auto &routing_data = routing_table_[address];

    bool const is_empty = (routing_data.handle == 0);

    // an update is only valid when the connection is direct.
    bool const is_connection_update = (routing_data.handle != handle);
    bool const is_duplicate_direct  = direct && routing_data.direct && is_connection_update;
    bool const is_upgrade           = (!is_empty) && (!routing_data.direct) && direct;
    bool const is_downgrade         = (!is_empty) && routing_data.direct && !direct;
    bool const is_different =
        (is_connection_update && !is_duplicate_direct && !is_downgrade) || is_upgrade;
    bool const is_update = (routing_data.handle && is_different);

    // update the routing table if required
    if (is_duplicate_direct)
    {
      FETCH_LOG_INFO(logging_name_, "Duplicate direct (detected)");

      // we do not overwrite the routing table for additional direct connections
      status = UpdateStatus::DUPLICATE_DIRECT;
    }
    else if (is_empty || is_update)
    {
      // replacing an existing entry
      Handle prev_handle = routing_data.handle;

      // update the table
      routing_data.handle = handle;
      routing_data.direct = direct;

      // signal an update was made to the table
      status  = UpdateStatus::UPDATED;
      display = is_empty || is_upgrade;

      FETCH_LOG_TRACE(logging_name_, is_connection_update, "-", is_duplicate_direct, "-",
                      is_upgrade, "-", is_different, "-", is_update);
      FETCH_LOG_TRACE(logging_name_, "Handle was: ", prev_handle, " now: ", handle,
                      " direct: ", direct, "-", routing_data.direct);
      FETCH_LOG_VARIABLE(prev_handle);
    }
  }

  if (display)
  {
    FETCH_LOG_INFO(logging_name_, "Adding ", ((direct) ? "direct" : "normal"),
                   " route for: ", ToBase64(ToConstByteArray(address)));
  }

  return status;
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

void Router::SetKademliaRouting(bool enable)
{
  kademlia_routing_ = enable;
}

/**
 * Looks up a random handle from the routing table.
 * @param address paremeter not used
 * @return The random handle, or zero if the routing table is empty
 */
Router::Handle Router::LookupRandomHandle(Packet::RawAddress const & /*address*/) const
{
  static std::random_device rd;
  static std::mt19937       rng(rd());

  {
    FETCH_LOCK(routing_table_lock_);

    if (!routing_table_.empty())
    {
      // decide the random index to access
      std::uniform_int_distribution<decltype(routing_table_)::size_type> distro(
          0, routing_table_.size() - 1);
      std::size_t const element = distro(rng);

      // advance the iterator to the correct offset
      auto it = routing_table_.cbegin();
      std::advance(it, static_cast<std::ptrdiff_t>(element));

      return it->second.handle;
    }
  }

  return 0;
}

/**
 * Lookup the closest directly connected handle to route the packet to
 *
 * @param address The address
 * @return
 */
Router::Handle Router::LookupKademliaClosestHandle(Address const &address) const
{
  Handle handle{0};

  auto const directly_connected = GetDirectlyConnectedPeerSet();
  if (!directly_connected.empty())
  {
    auto it   = directly_connected.begin();
    auto node = *it++;

    uint64_t best_distance = CalculateDistance(address, node);
    for (; it != directly_connected.end(); ++it)
    {
      uint64_t const distance = CalculateDistance(address, *it);

      if (distance < best_distance)
      {
        node = *it;
      }
    }

    handle = LookupHandle(ConvertAddress(node));
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
      dispatcher_.NotifyMessage(handle, packet->GetService(), packet->GetChannel(),
                                packet->GetMessageNum());
    }

    // serialize the packet to the buffer
    ByteArray buffer;
    buffer.Resize(packet->GetPacketSize());
    if (Packet::ToBuffer(*packet, buffer.pointer(), buffer.size()))
    {
      FETCH_LOG_TRACE(logging_name_, "TX: (conn: ", handle, ") ", DescribePacket(*packet));

      // dispatch to the connection object
      conn->Send(buffer);
    }
    else
    {
      FETCH_LOG_WARN(logging_name_, "Failed to generate binary stream for packet");
    }
  }
  else
  {
    FETCH_LOG_WARN(logging_name_, "Unable to route packet to handle: ", handle);
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

  // black list support

  /// Step 1. Determine if we should drop this packet (for whatever reason)
  if (external)
  {
    FETCH_LOG_TRACE(logging_name_, "Routing external packet.");
    // Handle TTL based routing timeout
    if (packet->GetTTL() <= 2u)
    {
      FETCH_LOG_WARN(logging_name_, "Message has timed out (TTL): ", DescribePacket(*packet));
      return;
    }
    // decrement the TTL
    packet->SetTTL(static_cast<uint8_t>(packet->GetTTL() - 1u));

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
    FETCH_LOG_TRACE(logging_name_, "Routing packet.");
    if (packet->GetSender() != address_)
    {
      DispatchPacket(packet, address_);
    }

    // serialize the packet to the buffer
    ByteArray buffer{};
    buffer.Resize(packet->GetPacketSize());
    if (Packet::ToBuffer(*packet, buffer.pointer(), buffer.size()))
    {
      FETCH_LOG_TRACE(logging_name_, "BX:           ", DescribePacket(*packet));

      // broadcast the data across the network
      register_.Broadcast(buffer);
    }
    else
    {
      FETCH_LOG_WARN(logging_name_, "Failed to serialise muddle packet to stream");
    }
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

    if (kademlia_routing_)
    {
      handle = LookupKademliaClosestHandle(packet->GetTarget());
      return;
    }

    // if direct routing fails then randomly select a handle. In future a better routing scheme
    // should be implemented.
    handle = LookupRandomHandle(packet->GetTargetRaw());
    if (handle)
    {
      FETCH_LOG_WARN(logging_name_, "Speculative routing to peer: ", ToBase64(packet->GetTarget()));
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
  FETCH_LOG_TRACE(logging_name_, "==> Direct message sent to router");

  // dispatch to the direct message handler if needed
  if (direct_message_handler_)
  {
    direct_message_handler_(handle, packet);
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

    FETCH_LOG_WARN(logging_name_,
                   "Unable to locate handler for routed message. Net: ", packet->GetNetworkId(),
                   " Service: ", packet->GetService(), " Channel: ", packet->GetChannel());
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

    if (delta > std::chrono::seconds{600})
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

}  // namespace muddle
}  // namespace fetch
