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

#include "kademlia/peer_tracker.hpp"
#include "muddle_logging_name.hpp"
#include "muddle_register.hpp"
#include "router.hpp"
#include "routing_message.hpp"

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

  static_assert(sizeof(out) == decltype(hash)::SIZE_IN_BYTES,
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
Router::Router(NetworkId network_id, Address address, MuddleRegister &reg, Prover const &prover)
  : name_{GenerateLoggingName(BASE_NAME, network_id)}
  , address_(std::move(address))
  , address_raw_(ConvertAddress(address_))
  , register_(reg)
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
  , informed_routing_total_(CreateCounter("ledger_router_informed_routing_total",
                                          "The total number of informed routed packets"))
  , speculative_routing_total_(CreateCounter("ledger_router_speculative_routing_total",
                                             "The total number of speculatively routed packets"))
  , failed_routing_total_(
        CreateCounter("ledger_router_failed_routing_total",
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
  stopping_ = false;
}

/**
 * Stops the routers internal dispatch thread pool
 */
void Router::Stop()
{
  stopping_ = true;

  {
    FETCH_LOCK(delivery_attempts_lock_);
    delivery_attempts_.clear();
  }

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
    // if this message does not belong to us we must route it along the path
    RoutePacket(packet);
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
  uint16_t const counter = GetNextCounter();

  Send(address, service, channel, counter, message, OPTION_DEFAULT);
}

void Router::Send(Address const &address, uint16_t service, uint16_t channel,
                  Payload const &message, Options options)
{
  uint16_t const counter = GetNextCounter();

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
  uint16_t const counter = GetNextCounter();

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
  if (!tracker_)
  {
    return {};
  }
  return tracker_->directly_connected_peers();
}

/**
 * Internal: Looks up the specified connection handle from a given address
 *
 * @param address The address to look up the handle for.
 * @return The target handle for the connection, or zero on failure.
 */
Router::Handle Router::LookupHandle(Packet::RawAddress const &raw_address) const
{
  if (!tracker_)
  {
    FETCH_LOG_ERROR(logging_name_, "Tracker not set. Unable to lookup address.");
    return 0;
  }

  auto address = ConvertAddress(raw_address);
  return tracker_->LookupHandle(address);
}

/**
 * Internal: Takes a given packet and sends it to the connection specified by the handle
 *
 * @param handle The handle to the network connection
 * @param packet The packet to be routed
 */
void Router::SendToConnection(Handle handle, PacketPtr const &packet, bool external,
                              bool reschedule_on_fail)
{
  // internal method, we expect all inputs be valid at this stage
  assert(static_cast<bool>(packet));

  // Callbacks to deal with package redelivery
  std::weak_ptr<Packet> wpacket = packet;
  auto                  success = [this, wpacket]() {
    auto packet = wpacket.lock();
    if (packet)
    {
      ClearDeliveryAttempt(packet);
    }
  };

  auto fail = [this, wpacket, external, reschedule_on_fail]() {
    auto packet = wpacket.lock();
    if (packet && reschedule_on_fail)
    {
      SchedulePacketForRedelivery(packet, external);
    }
  };

  // look up the connection
  auto conn = register_.LookupConnection(handle).lock();
  if (conn)
  {
    // serialize the packet to the buffer
    ByteArray buffer;
    buffer.Resize(packet->GetPacketSize());
    if (Packet::ToBuffer(*packet, buffer.pointer(), buffer.size()))
    {
      FETCH_LOG_TRACE(logging_name_, "TX: (conn: ", handle, ") ", DescribePacket(*packet));

      // dispatch to the connection object
      conn->Send(buffer, success, fail);

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
    if (reschedule_on_fail)
    {
      // Rescheduling
      SchedulePacketForRedelivery(packet, external);
    }
    else
    {
      FETCH_LOG_WARN(logging_name_, "Unable to route packet to handle: ", handle);
    }
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

      ClearDeliveryAttempt(packet);
      return;
    }
    // decrement the TTL
    packet->SetTTL(static_cast<uint8_t>(packet->GetTTL() - 1u));

    // if this packet is a broadcast echo we should no longer route this packet
    if (packet->IsBroadcast() && IsEcho(*packet))
    {
      ClearDeliveryAttempt(packet);
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

    ClearDeliveryAttempt(packet);
  }
  else
  {
    // attempt to route to one of our direct peers
    Handle handle = LookupHandle(packet->GetTargetRaw());
    if (handle != 0u)
    {
      // one of our direct connections is the target address, route and complete
      SendToConnection(handle, packet, external, true);
      normal_routing_total_->increment();

      // ClearDeliveryAttempt is done by SendToConnection
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

        SendToConnection(handle, packet, external, true);
        informed_routing_total_->increment();

        // ClearAttempts is done by SendToConnection
        return;
      }

      FETCH_LOG_ERROR(logging_name_, "Informed routing; Invalid handle");
    }

    // Scheduling for redelivery
    SchedulePacketForRedelivery(packet, external);
    return;
  }
}

void Router::SchedulePacketForRedelivery(PacketPtr const &packet, bool external)
{
  // If the router is stopping we do not attempt redelivery
  if (stopping_)
  {
    ClearDeliveryAttempt(packet);
    return;
  }

  // Taking note of packet attempted delivery
  // This is only suppose to happen in extraordinary circumstansed
  uint64_t attempts{0};
  {
    FETCH_LOCK(delivery_attempts_lock_);

    if (delivery_attempts_.find(packet) == delivery_attempts_.end())
    {
      delivery_attempts_[packet] = 0;

      // Ensuring that the tracker is looking for the desired connection
      tracker_->AddDesiredPeer(packet->GetTarget(), config_.temporary_connection_length);
    }

    attempts = ++delivery_attempts_[packet];
  }

  // Giving up
  if (attempts > config_.max_delivery_attempts)
  {
    // As a last resort we just deliver the message to a random peer
    ClearDeliveryAttempt(packet);

    // if direct routing fails then randomly select a handle. In future a better routing scheme
    // should be implemented.
    auto handle = tracker_->LookupRandomHandle();
    if (handle != 0u)
    {
      FETCH_LOG_WARN(logging_name_,
                     "Speculative routing to peer: ", packet->GetTarget().ToBase64());
      SendToConnection(handle, packet, external, false);
      speculative_routing_total_->increment();
      return;
    }

    FETCH_LOG_ERROR(logging_name_, "Unable to route packet to: ", packet->GetTarget().ToBase64());
    failed_routing_total_->increment();

    return;
  }

  // Retrying at a later point
  FETCH_LOG_DEBUG(logging_name_, "Retrying packet delivery: ", packet->GetTarget().ToBase64());

  if (!stopping_)
  {
    dispatch_thread_pool_->Post(
        [this, packet, external]() {
          if (stopping_)
          {
            return;
          }
          // We delibrately set external to false to not update TTL and echo filter again
          RoutePacket(packet, external);
        },
        config_.retry_delay_ms);
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

  if (!stopping_)
  {
    dispatch_thread_pool_->Post([this, packet, handle]() {
      if (stopping_)
      {
        return;
      }

      // Updating the association between handle and address
      if (register_.UpdateAddress(handle, packet->GetSender()) ==
          MuddleRegister::UpdateStatus::NEW_ADDRESS)
      {
        dispatch_thread_pool_->Post([this, packet, handle]() {
          tracker_->DownloadPeerDetails(handle, packet->GetSender());
        });
      }

      // dispatch to the direct message handler if needed
      if (direct_message_handler_)
      {
        direct_message_handler_(handle, packet);
      }
      else
      {
        dispatch_failure_total_->increment();
      }

      dispatch_complete_total_->increment();
    });
  }
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
