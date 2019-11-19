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
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/fnv.hpp"
#include "crypto/secure_channel.hpp"
#include "logging/logging.hpp"
#include "muddle/packet.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"

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

  static_assert(sizeof(out) == decltype(hash)::size_in_bytes,
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

telemetry::Measurement::Labels CreateLabels(Router const &router)
{
  telemetry::Measurement::Labels labels{};
  labels["network"] = router.network().ToString();
  labels["address"] = static_cast<std::string>(router.network_address().ToBase64());
  return labels;
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
               Prover const &prover)
  : name_{GenerateLoggingName(BASE_NAME, network_id)}
  , address_(std::move(address))
  , address_raw_(ConvertAddress(address_))
  , register_(reg)
  , dispatcher_(dispatcher)
  , registrar_(network_id)
  , network_id_(network_id)
  , prover_(prover)
  , dispatch_thread_pool_(network::MakeThreadPool(NUMBER_OF_ROUTER_THREADS, "Router"))
  , rx_max_packet_length(
        CreateGauge("ledger_router_rx_max_packet_length", "The max received packet length"))
  , tx_max_packet_length(
        CreateGauge("ledger_router_tx_max_packet_length", "The max transmitted packet length"))
  , bx_max_packet_length(
        CreateGauge("ledger_router_bx_max_packet_length", "The max broadcasted packet length"))
  , rx_packet_length(CreateHistogram("ledger_router_rx_packet_length",
                                     "The histogram of received packet lengths"))
  , tx_packet_length(CreateHistogram("ledger_router_tx_packet_length",
                                     "The histogram of transmitted packet lengths"))
  , bx_packet_length(CreateHistogram("ledger_router_bx_packet_length",
                                     "The histogram of broadcasted packet lengths"))
  , rx_packet_total_(
        CreateCounter("ledger_router_rx_packet_total", "The total number of received packets"))
  , tx_packet_total_(
        CreateCounter("ledger_router_tx_packet_total", "The total number of transmitted packets"))
  , bx_packet_total_(
        CreateCounter("ledger_router_bx_packet_total", "The total number of broadcasted packets"))
  , rx_encrypted_packet_failures_total_(
        (CreateCounter("ledger_router_rx_encrypted_packet_failures_total",
                       "The total number of received encrypted packets that could not be read")))
  , rx_encrypted_packet_success_total_(
        (CreateCounter("ledger_router_rx_encrypted_packet_success_total",
                       "The total number of received encrypted packets that could be read")))
  , tx_encrypted_packet_failures_total_(
        (CreateCounter("ledger_router_tx_encrypted_packet_failures_total",
                       "The total number of sent encrypted packets that could not be generated")))
  , tx_encrypted_packet_success_total_(
        (CreateCounter("ledger_router_tx_encrypted_packet_success_total",
                       "The total number of sent encrypted packets that could be generated")))
  , ttl_expired_packet_total_(
        CreateCounter("ledger_router_ttl_expired_packet_total",
                      "The total number of packets that have expired due to TTL"))
  , dispatch_enqueued_total_(CreateCounter("ledger_router_enqueued_packet_total",
                                           "The total number of enqueued packets to be dispatched"))
  , exchange_dispatch_total_(CreateCounter("ledger_router_exchange_packet_total",
                                           "The total number of exchange packets dispatched"))
  , subscription_dispatch_total_(
        CreateCounter("ledger_router_subscription_packet_total",
                      "The total number of subscription packets dispatched"))
  , dispatch_direct_total_(CreateCounter("ledger_router_direct_packet_total",
                                         "The total number of direct packets dispatched"))
  , dispatch_failure_total_(CreateCounter("ledger_router_dispatch_failure_total",
                                          "The total number of dispatch failures"))
  , dispatch_complete_total_(CreateCounter("ledger_router_dispatch_complete_total",
                                           "The total number of completed dispatchs"))
  , foreign_packet_total_(
        CreateCounter("ledger_router_foreign_packet_total", "The total number of foreign packets"))
  , fraudulent_packet_total_(CreateCounter("ledger_router_fraudulent_packet_total",
                                           "The total number of fraudulent packets"))
  , routing_table_updates_total_(CreateCounter("ledger_router_table_updates_total",
                                               "The total number of updates to the routing table"))
  , echo_cache_trims_total_(CreateCounter("ledger_router_echo_cache_trims_total",
                                          "The total number of times the echo cache was trimmed"))
  , echo_cache_removals_total_(
        CreateCounter("ledger_router_echo_cache_removal_total",
                      "The total number of entries removed from the echo cache"))
  , normal_routing_total_(CreateCounter("ledger_router_normal_routing_total",
                                        "The total number of normally routed packets"))
  , informed_routing_total_(CreateCounter("ledger_router_normal_routing_total",
                                          "The total number of informed routed packets"))
  , kademlia_routing_total_(CreateCounter("ledger_router_normal_routing_total",
                                          "The total number of kademlia routed packets"))
  , speculative_routing_total_(CreateCounter("ledger_router_normal_routing_total",
                                             "The total number of speculatively routed packets"))
  , failed_routing_total_(
        CreateCounter("ledger_router_normal_routing_total",
                      "The total number of packets that have failed to be routed"))
  , connection_dropped_total_(CreateCounter("ledger_router_connection_dropped_total",
                                            "The total number of connections dropped"))
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
  bool genuine{true};

  if (p->IsStamped() || p->IsBroadcast())
  {
    genuine = p->Verify();
  }

  return genuine;
}

Router::PacketPtr const &Router::Sign(PacketPtr const &p) const
{
  p->Sign(prover_);

  return p;
}

/**
 * Takes an input packet from the network layer and routes it across the network
 *
 * @param handle The handle of the receiving connection for the packet
 * @param packet The input packet to route
 */
void Router::Route(Handle handle, PacketPtr const &packet)
{
  FETCH_LOG_TRACE(logging_name_, "RX: (conn: ", handle, ") ", DescribePacket(*packet));

  // input packet size information
  uint64_t const packet_size = packet->GetPacketSize();
  rx_packet_total_->increment();
  rx_max_packet_length->max(packet_size);
  rx_packet_length->Add(static_cast<double>(packet_size));

  // discard all foreign packets
  if (packet->GetNetworkId() != network_id_.value())
  {
    FETCH_LOG_WARN(logging_name_, "Discarding foreign packet: ", DescribePacket(*packet), " at ",
                   ToBase64(address_), ":", network_id_.ToString());

    foreign_packet_total_->increment();
    return;
  }

  if (!Genuine(packet))
  {
    FETCH_LOG_WARN(logging_name_, "Packet's authenticity not verified:", DescribePacket(*packet));
    fraudulent_packet_total_->increment();
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
    AssociateHandleWithAddress(handle, packet->GetSenderRaw(), false, packet->IsBroadcast());

    // if this message does not belong to us we must route it along the path
    RoutePacket(packet);
  }
}

void Router::ConnectionDropped(Handle handle)
{
  FETCH_LOG_INFO(logging_name_, "Connection ", handle, " dropped");
  connection_dropped_total_->add(1);

  FETCH_LOCK(routing_table_lock_);
  for (auto it = routing_table_.begin(); it != routing_table_.end();)
  {
    auto &handles = it->second.handles;

    for (auto entry_it = handles.begin(); entry_it != handles.end();)
    {
      if (*entry_it == handle)
      {
        // remove the handle from the vector
        entry_it = handles.erase(entry_it);
      }
      else
      {
        // move along handle in the connection list
        ++entry_it;
      }
    }

    if (handles.empty())
    {
      // this address no longer has any handles associated with it, therefore it should be removed
      it = routing_table_.erase(it);
    }
    else
    {
      // move along to the next entry in the routing table
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

  if ((options & OPTION_EXCHANGE) != 0u)
  {
    packet->SetExchange(true);
  }

  if ((options & OPTION_ENCRYPTED) != 0u)
  {
    ConstByteArray encrypted_payload{};
    bool const     encrypted = secure_channel_.Encrypt(address, service, channel, message_num,
                                                   packet->GetPayload(), encrypted_payload);
    if (!encrypted)
    {
      FETCH_LOG_ERROR(logging_name_, "Unable to encrypt packet contents");
      tx_encrypted_packet_failures_total_->increment();
      return;
    }

    packet->SetPayload(encrypted_payload);
    packet->SetEncrypted(true);
    tx_encrypted_packet_success_total_->increment();
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
                                                        bool direct, bool broadcast)
{
  UpdateStatus status{UpdateStatus::NO_CHANGE};

  // At the moment these updates (and by extension the routing logic) works on a first
  // come first served basis. This is not reliable longer term and will need tweaking

  // sanity check
  assert(handle);

  if (broadcast)
  {
    FETCH_LOG_TRACE(logging_name_, "Ignoring routing messages from broadcast");
    return status;
  }

  // never allow the current node address to be added to the routing table
  if (address == address_raw_)
  {
    return status;
  }

  bool display{false};

  FETCH_LOCK(routing_table_lock_);

  // lookup (or create) the routing table entry
  auto &routing_data = routing_table_[address];

  bool const is_empty = (routing_data.handles.empty());

  // cache the previous handle
  Handle const current_handle = (is_empty) ? 0 : routing_data.handles.front();

  // an update is only valid when the connection is direct.
  bool const is_connection_update = (current_handle != handle);
  bool const is_duplicate_direct  = direct && routing_data.direct && is_connection_update;
  bool const is_upgrade           = (!is_empty) && (!routing_data.direct) && direct;
  bool const is_downgrade         = (!is_empty) && routing_data.direct && !direct;
  bool const is_different =
      (is_connection_update && !is_duplicate_direct && !is_downgrade) || is_upgrade;
  bool const is_update = ((!is_empty) && is_different);

  // update the routing table if required
  if (is_duplicate_direct)
  {
    FETCH_LOG_INFO(logging_name_, "Duplicate direct (detected) conn: ", handle);

    // add the handle to the list of available
    routing_data.handles.emplace_back(handle);

    routing_table_updates_total_->increment();

    // we do not overwrite the routing table for additional direct connections
    status = UpdateStatus::DUPLICATE_DIRECT;
  }
  else if (is_empty || is_update)
  {
    // update the table
    routing_data.handles.assign(1, handle);
    routing_data.direct = direct;

    // signal an update was made to the table
    status  = UpdateStatus::UPDATED;
    display = is_empty || is_upgrade;

    routing_table_updates_total_->increment();

    FETCH_LOG_TRACE(logging_name_, is_connection_update, "-", is_duplicate_direct, "-", is_upgrade,
                    "-", is_different, "-", is_update);
    FETCH_LOG_TRACE(logging_name_, "Handle was: ", current_handle, " now: ", handle,
                    " direct: ", direct, "-", routing_data.direct);
    FETCH_LOG_VARIABLE(current_handle);
  }

  if (display)
  {
    FETCH_LOG_INFO(logging_name_, "Adding ", ((direct) ? "direct" : "normal"),
                   " route for: ", ConvertAddress(address).ToBase64(), " (handle: ", handle, ")");
  }

  return status;
}

/**
 * Internal: Looks up the specified connection handle from a given address
 *
 * @param address The address to look up the handle for.
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

      if (!routing_data.handles.empty())
      {
        handle = routing_data.handles.front();
      }
    }
  }

  FETCH_LOG_TRACE(logging_name_, "Routing decision: ", ConvertAddress(address).ToBase64(), " -> ",
                  handle);

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
  thread_local std::random_device rd;
  thread_local std::mt19937       rng(rd());

  {
    FETCH_LOCK(routing_table_lock_);

    if (!routing_table_.empty())
    {
      using Distribution = std::uniform_int_distribution<std::size_t>;

      // decide the random index to access
      Distribution      table_dist{0, routing_table_.size() - 1};
      std::size_t const element = table_dist(rng);

      // advance the iterator to the correct offset
      auto it = routing_table_.cbegin();
      std::advance(it, static_cast<std::ptrdiff_t>(element));

      auto const &handles = it->second.handles;

      if (!handles.empty())
      {
        Distribution handle_dist{0, handles.size() - 1};
        return handles[handle_dist(rng)];
      }
    }
  }

  return 0;
}

/**
 * Look up the closest directly connected handle to route the packet to
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
void Router::SendToConnection(Handle handle, PacketPtr const &packet)
{
  // internal method, we expect all inputs be valid at this stage
  assert(static_cast<bool>(packet));

  // look up the connection
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

      tx_packet_total_->increment();
      tx_max_packet_length->max(buffer.size());
      tx_packet_length->Add(static_cast<double>(buffer.size()));
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
void Router::RoutePacket(PacketPtr const &packet, bool external)
{
  // black list support

  /// Step 1. Determine if we should drop this packet (for whatever reason)
  if (external)
  {
    FETCH_LOG_TRACE(logging_name_, "Routing external packet.");

    // Handle TTL based routing timeout
    if (packet->GetTTL() <= 2u)
    {
      ttl_expired_packet_total_->increment();

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
      bx_packet_total_->increment();
      bx_max_packet_length->max(buffer.size());
      bx_packet_length->Add(static_cast<double>(buffer.size()));
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
    if (handle != 0u)
    {
      // one of our direct connections is the target address, route and complete
      SendToConnection(handle, packet);
      normal_routing_total_->increment();
      return;
    }

    // this should never be necessary, but in the case where the routing table is not correctly
    // updated by the peer is directly connected, then we should always use that peer
    auto const address_index = register_.GetAddressIndex();
    auto const index_it      = address_index.find(packet->GetTarget());
    if (index_it != address_index.end())
    {
      // extract the handle from the index
      handle = (index_it->second) ? index_it->second->handle : 0u;

      if (handle != 0u)
      {
        FETCH_LOG_WARN(logging_name_, "Informed routing to peer: ", packet->GetTarget().ToBase64());

        SendToConnection(handle, packet);
        informed_routing_total_->increment();
        return;
      }

      FETCH_LOG_ERROR(logging_name_, "Informed routing; Invalid handle");
    }

    // if kad routing is enabled we should use this to route packets
    if (kademlia_routing_)
    {
      handle = LookupKademliaClosestHandle(packet->GetTarget());
      if (handle != 0u)
      {
        SendToConnection(handle, packet);
        kademlia_routing_total_->increment();
        return;
      }
    }

    // if direct routing fails then randomly select a handle. In future a better routing scheme
    // should be implemented.
    handle = LookupRandomHandle(packet->GetTargetRaw());
    if (handle != 0u)
    {
      FETCH_LOG_WARN(logging_name_,
                     "Speculative routing to peer: ", packet->GetTarget().ToBase64());
      SendToConnection(handle, packet);
      speculative_routing_total_->increment();
      return;
    }

    FETCH_LOG_ERROR(logging_name_, "Unable to route packet to: ", packet->GetTarget().ToBase64());
    failed_routing_total_->increment();
  }
}

/**
 * Dispatch / Handle the direct packet from a single hop peer
 *
 * @param handle The handle to the originating connection
 * @param packet The packet that was received
 */
void Router::DispatchDirect(Handle handle, PacketPtr const &packet)
{
  FETCH_LOG_TRACE(logging_name_, "==> Direct message sent to router");
  dispatch_enqueued_total_->increment();

  dispatch_thread_pool_->Post([this, packet, handle]() {
    // dispatch to the direct message handler if needed
    if (direct_message_handler_)
    {
      direct_message_handler_(handle, packet);
      dispatch_direct_total_->increment();
    }

    dispatch_failure_total_->increment();
    dispatch_complete_total_->increment();
  });
}

/**
 * Dispatch / Handle a normally (routed) packet
 *
 * @param packet The packet that was received
 */
void Router::DispatchPacket(PacketPtr const &packet, Address const &transmitter)
{
  dispatch_enqueued_total_->increment();

  dispatch_thread_pool_->Post([this, packet, transmitter]() {
    bool const isPossibleExchangeResponse = !packet->IsExchange();

    // decrypt encrypted messages
    if (packet->IsEncrypted())
    {
      ConstByteArray decrypted_payload{};
      bool const     decrypted =
          secure_channel_.Decrypt(packet->GetSender(), packet->GetService(), packet->GetChannel(),
                                  packet->GetMessageNum(), packet->GetPayload(), decrypted_payload);

      if (!decrypted)
      {
        FETCH_LOG_ERROR(logging_name_, "Unable to decrypt input message");
        rx_encrypted_packet_failures_total_->increment();
        return;
      }

      // update the payload to be decrypted payload
      packet->SetPayload(decrypted_payload);
      rx_encrypted_packet_success_total_->increment();
    }

    // determine if this was an exchange based node
    if (isPossibleExchangeResponse && dispatcher_.Dispatch(packet))
    {
      exchange_dispatch_total_->increment();
      dispatch_complete_total_->increment();

      // the dispatcher has "claimed" this packet as there was an outstanding promise waiting for it
      return;
    }

    // If no exchange message has claimed this then attempt to dispatch it through our normal system
    // of message subscriptions.
    if (registrar_.Dispatch(packet, transmitter))
    {
      subscription_dispatch_total_->increment();
      dispatch_complete_total_->increment();
      return;
    }

    FETCH_LOG_WARN(logging_name_,
                   "Unable to locate handler for routed message. Net: ", packet->GetNetworkId(),
                   " Service: ", packet->GetService(), " Channel: ", packet->GetChannel());

    dispatch_failure_total_->increment();
    dispatch_complete_total_->increment();
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

    // look up if the echo is in the cache
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

  echo_cache_trims_total_->increment();

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

      echo_cache_removals_total_->increment();
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

Router::RoutingTable Router::routing_table() const
{
  FETCH_LOCK(routing_table_lock_);
  return routing_table_;
}

Router::EchoCache Router::echo_cache() const
{
  FETCH_LOCK(echo_cache_lock_);
  return echo_cache_;
}

NetworkId const &Router::network() const
{
  return network_id_;
}

Address const &Router::network_address() const
{
  return address_;
}

telemetry::GaugePtr<uint64_t> Router::CreateGauge(char const *name, char const *description) const
{
  return telemetry::Registry::Instance().CreateGauge<uint64_t>(name, description,
                                                               CreateLabels(*this));
}

telemetry::HistogramPtr Router::CreateHistogram(char const *name, char const *description) const
{
  return telemetry::Registry::Instance().CreateHistogram(
      {1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9}, name, description, CreateLabels(*this));
}

telemetry::CounterPtr Router::CreateCounter(char const *name, char const *description) const
{
  return telemetry::Registry::Instance().CreateCounter(name, description, CreateLabels(*this));
}

}  // namespace muddle
}  // namespace fetch
