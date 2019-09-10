#pragma once
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

#include "blacklist.hpp"
#include "subscription_registrar.hpp"

#include "core/mutex.hpp"
#include "crypto/prover.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/network_id.hpp"
#include "muddle/packet.hpp"
#include "network/details/thread_pool.hpp"
#include "network/management/abstract_connection.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace muddle {

class Dispatcher;
class MuddleRegister;

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
  using Handle               = network::AbstractConnection::connection_handle_type;
  using ThreadPool           = network::ThreadPool;
  using HandleDirectAddrMap  = std::unordered_map<Handle, Address>;
  using Prover               = crypto::Prover;
  using DirectMessageHandler = std::function<void(Handle, PacketPtr)>;

  struct RoutingData
  {
    bool   direct = false;
    Handle handle = 0;
  };

  using RoutingTable = std::unordered_map<Packet::RawAddress, RoutingData>;

  // Helper functions
  static Packet::RawAddress ConvertAddress(Packet::Address const &address);
  static Packet::Address    ConvertAddress(Packet::RawAddress const &address);

  // Construction / Destruction
  Router(NetworkId network_id, Address address, MuddleRegister &reg, Dispatcher &dispatcher,
         Prover *certificate = nullptr, bool sign_broadcasts = false);
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

  void Route(Handle handle, PacketPtr packet);
  void ConnectionDropped(Handle handle);

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

  Response Exchange(Address const &address, uint16_t service, uint16_t channel,
                    Payload const &request) override;

  SubscriptionPtr Subscribe(uint16_t service, uint16_t channel) override;
  SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel) override;

  AddressList GetDirectlyConnectedPeers() const override;

  AddressSet GetDirectlyConnectedPeerSet() const override;

  /// @}

  void Cleanup();

  void Blacklist(Address const &target);
  void Whitelist(Address const &target);
  bool IsBlacklisted(Address const &target) const;

  Handle LookupHandle(Packet::RawAddress const &address) const;

  void SetDirectHandler(DirectMessageHandler handler)
  {
    direct_message_handler_ = std::move(handler);
  }

  void SetKademliaRouting(bool enable = true);

  // Operators
  Router &operator=(Router const &) = delete;
  Router &operator=(Router &&) = delete;

private:
  using HandleMap  = std::unordered_map<Handle, std::unordered_set<Packet::RawAddress>>;
  using Clock      = std::chrono::steady_clock;
  using Timepoint  = Clock::time_point;
  using EchoCache  = std::unordered_map<std::size_t, Timepoint>;
  using RawAddress = Packet::RawAddress;
  using BlackList  = fetch::muddle::Blacklist;

  enum class UpdateStatus
  {
    NO_CHANGE,
    DUPLICATE_DIRECT,
    UPDATED
  };

  static constexpr std::size_t NUMBER_OF_ROUTER_THREADS = 1;

  UpdateStatus AssociateHandleWithAddress(Handle handle, Packet::RawAddress const &address,
                                          bool direct);

  Handle LookupRandomHandle(Packet::RawAddress const &address) const;
  Handle LookupKademliaClosestHandle(Address const &address) const;

  void SendToConnection(Handle handle, PacketPtr packet);
  void RoutePacket(PacketPtr packet, bool external = true);
  void DispatchDirect(Handle handle, PacketPtr packet);

  void DispatchPacket(PacketPtr packet, Address transmitter);

  bool IsEcho(Packet const &packet, bool register_echo = true);
  void CleanEchoCache();

  PacketPtr const &Sign(PacketPtr const &p) const;
  bool             Genuine(PacketPtr const &p) const;

  std::string const     name_;
  char const *const     logging_name_{name_.c_str()};
  Address const         address_;
  RawAddress const      address_raw_;
  MuddleRegister &      register_;
  DirectMessageHandler  direct_message_handler_;
  BlackList             blacklist_;
  Dispatcher &          dispatcher_;
  SubscriptionRegistrar registrar_;
  NetworkId             network_id_;
  Prover *              prover_          = nullptr;
  bool                  sign_broadcasts_ = false;
  std::atomic<bool>     kademlia_routing_{false};

  mutable Mutex routing_table_lock_{__LINE__, __FILE__};
  RoutingTable  routing_table_;  ///< The map routing table from address to handle (Protected by

  mutable Mutex echo_cache_lock_{__LINE__, __FILE__};
  EchoCache     echo_cache_;

  ThreadPool dispatch_thread_pool_;

  friend class DirectMessageService;
};

}  // namespace muddle
}  // namespace fetch
