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

#include "blacklist.hpp"
#include "subscription_registrar.hpp"

#include "core/mutex.hpp"
#include "crypto/prover.hpp"
#include "crypto/secure_channel.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/network_id.hpp"
#include "muddle/packet.hpp"
#include "muddle/router_configuration.hpp"
#include "network/details/thread_pool.hpp"
#include "network/management/abstract_connection.hpp"
#include "telemetry/telemetry.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace muddle {

class MuddleRegister;
class PeerTracker;

/**
 * The router if the fundamental object of the muddle system an routes external and internal packets
 * to either a subscription or to another node on the network
 */
class Router : public MuddleEndpoint
{
public:
  using Address              = Packet::Address;  // == a crypto::Identity.identifier_
  using PacketPtr            = std::shared_ptr<Packet>;
  using Payload              = Packet::Payload;
  using ConnectionPtr        = std::weak_ptr<network::AbstractConnection>;
  using Handle               = network::AbstractConnection::ConnectionHandleType;
  using ThreadPool           = network::ThreadPool;
  using HandleDirectAddrMap  = std::unordered_map<Handle, Address>;
  using Prover               = crypto::Prover;
  using DirectMessageHandler = std::function<void(Handle, PacketPtr)>;
  using Handles              = std::vector<Handle>;
  using PeerTrackerPtr       = std::shared_ptr<PeerTracker>;

  struct RoutingData
  {
    bool    direct = false;
    Handles handles{};
  };

  using RoutingTable = std::unordered_map<Packet::RawAddress, RoutingData>;
  using Clock        = std::chrono::steady_clock;
  using Timepoint    = Clock::time_point;
  using EchoCache    = std::unordered_map<std::size_t, Timepoint>;

  // Helper functions
  static Packet::RawAddress ConvertAddress(Packet::Address const &address);
  static Packet::Address    ConvertAddress(Packet::RawAddress const &address);

  // Construction / Destruction
  Router(NetworkId network_id, Address address, MuddleRegister &reg, Prover const &prover);
  Router(Router const &) = delete;
  Router(Router &&)      = delete;
  ~Router() override     = default;

  NetworkId const &network_id() const override
  {
    return network_id_;
  }

  // Start / Stop
  void Start();
  void Stop();

  void Route(Handle handle, PacketPtr const &packet);

  /// @name Endpoint Methods (Publicly visible)
  /// @{
  Address const &GetAddress() const override;

  void Send(Address const &address, uint16_t service, uint16_t channel,
            Payload const &message) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, Payload const &message,
            Options options) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload, Options options) override;

  void Broadcast(uint16_t service, uint16_t channel, Payload const &payload) override;

  SubscriptionPtr Subscribe(uint16_t service, uint16_t channel) override;
  SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel) override;

  AddressList GetDirectlyConnectedPeers() const override;

  AddressSet GetDirectlyConnectedPeerSet() const override;
  /// @}

  void Cleanup();

  void Blacklist(Address const &target);
  void Whitelist(Address const &target);
  bool IsBlacklisted(Address const &target) const;

  Handle LookupHandle(Packet::RawAddress const &raw_address) const;

  void SetDirectHandler(DirectMessageHandler handler)
  {
    direct_message_handler_ = std::move(handler);
  }

  void SetTracker(PeerTrackerPtr const &tracker)
  {
    tracker_ = tracker;
  }

  EchoCache        echo_cache() const;
  NetworkId const &network() const;
  Address const &  network_address() const;

  // Operators
  Router &operator=(Router const &) = delete;
  Router &operator=(Router &&) = delete;

private:
  using RawAddress = Packet::RawAddress;
  using BlackList  = fetch::muddle::Blacklist;

  enum class UpdateStatus
  {
    NO_CHANGE,
    DUPLICATE_DIRECT,
    UPDATED
  };

  static constexpr std::size_t NUMBER_OF_ROUTER_THREADS = 1;

  void SendToConnection(Handle handle, PacketPtr const &packet, bool external = true,
                        bool reschedule_on_fail = false);
  void RoutePacket(PacketPtr const &packet, bool external = true);
  void DispatchDirect(Handle handle, PacketPtr const &packet);

  void DispatchPacket(PacketPtr const &packet, Address const &transmitter);

  bool IsEcho(Packet const &packet, bool register_echo = true);
  void CleanEchoCache();

  PacketPtr const &Sign(PacketPtr const &p) const;
  bool             Genuine(PacketPtr const &p) const;

  telemetry::GaugePtr<uint64_t> CreateGauge(char const *name, char const *description) const;
  telemetry::HistogramPtr       CreateHistogram(char const *name, char const *description) const;
  telemetry::CounterPtr         CreateCounter(char const *name, char const *description) const;

  std::string const     name_;
  char const *const     logging_name_{name_.c_str()};
  Address const         address_;
  RawAddress const      address_raw_;
  MuddleRegister &      register_;
  DirectMessageHandler  direct_message_handler_;
  BlackList             blacklist_;
  SubscriptionRegistrar registrar_;
  NetworkId             network_id_;
  crypto::Prover const &prover_;
  crypto::SecureChannel secure_channel_{prover_};
  std::atomic<bool>     stopping_{false};
  RouterConfiguration   config_{};

  PeerTrackerPtr tracker_{nullptr};

  mutable Mutex echo_cache_lock_;
  EchoCache     echo_cache_;

  ThreadPool dispatch_thread_pool_;

  /// Redelivery of packages
  /// @{
  mutable Mutex                           delivery_attempts_lock_;
  std::unordered_map<PacketPtr, uint64_t> delivery_attempts_;

  void ClearDeliveryAttempt(PacketPtr const &packet)
  {
    FETCH_LOCK(delivery_attempts_lock_);
    delivery_attempts_.erase(packet);
  }
  void SchedulePacketForRedelivery(PacketPtr const &packet, bool external);
  /// @}

  /// Message "entropy"
  /// @{
  uint16_t GetNextCounter()
  {
    return counter_++;
  }

  std::atomic<uint16_t> counter_{0};
  /// @}

  /// Telemetry
  /// @{
  telemetry::GaugePtr<uint64_t> rx_max_packet_length;
  telemetry::GaugePtr<uint64_t> tx_max_packet_length;
  telemetry::GaugePtr<uint64_t> bx_max_packet_length;
  telemetry::HistogramPtr       rx_packet_length;
  telemetry::HistogramPtr       tx_packet_length;
  telemetry::HistogramPtr       bx_packet_length;
  telemetry::CounterPtr         rx_packet_total_;
  telemetry::CounterPtr         tx_packet_total_;
  telemetry::CounterPtr         bx_packet_total_;
  telemetry::CounterPtr         rx_encrypted_packet_failures_total_;
  telemetry::CounterPtr         rx_encrypted_packet_success_total_;
  telemetry::CounterPtr         tx_encrypted_packet_failures_total_;
  telemetry::CounterPtr         tx_encrypted_packet_success_total_;
  telemetry::CounterPtr         ttl_expired_packet_total_;
  telemetry::CounterPtr         dispatch_enqueued_total_;
  telemetry::CounterPtr         exchange_dispatch_total_;
  telemetry::CounterPtr         subscription_dispatch_total_;
  telemetry::CounterPtr         dispatch_direct_total_;
  telemetry::CounterPtr         dispatch_failure_total_;
  telemetry::CounterPtr         dispatch_complete_total_;
  telemetry::CounterPtr         foreign_packet_total_;
  telemetry::CounterPtr         fraudulent_packet_total_;
  telemetry::CounterPtr         routing_table_updates_total_;
  telemetry::CounterPtr         echo_cache_trims_total_;
  telemetry::CounterPtr         echo_cache_removals_total_;
  telemetry::CounterPtr         normal_routing_total_;
  telemetry::CounterPtr         informed_routing_total_;
  telemetry::CounterPtr         speculative_routing_total_;
  telemetry::CounterPtr         failed_routing_total_;
  telemetry::CounterPtr         connection_dropped_total_;
  /// @}

  friend class DirectMessageService;
};

}  // namespace muddle
}  // namespace fetch
