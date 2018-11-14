#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/mutex.hpp"
#include "network/details/thread_pool.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/subscription_registrar.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"
#include "network/muddle/blacklist.hpp"

#include <chrono>
#include <memory>

namespace fetch {
namespace muddle {

class Packet;

class Dispatcher;

class MuddleRegister;

/**
 * The router if the fundamental object of the muddle system an routes external and internal packets
 * to either a subscription or to another node on the network
 */
class Router : public MuddleEndpoint
{
public:
  using Address       = Packet::Address;  // == a crypto::Identity.identifier_
  using PacketPtr     = std::shared_ptr<Packet>;
  using Payload       = Packet::Payload;
  using ConnectionPtr = std::weak_ptr<network::AbstractConnection>;
  using Handle        = network::AbstractConnection::connection_handle_type;
  using ThreadPool    = network::ThreadPool;

  struct RoutingData
  {
    bool   direct = false;
    Handle handle = 0;
  };

  using RoutingTable = std::unordered_map<Packet::RawAddress, RoutingData>;

  static constexpr char const *LOGGING_NAME = "MuddleRoute";

  // Construction / Destruction
  Router(Address address, MuddleRegister const &reg, Dispatcher &dispatcher);
  Router(Router const &) = delete;
  Router(Router &&)      = delete;
  ~Router() override     = default;

  // Start / Stop
  void Start();
  void Stop();

  // Operators
  Router &operator=(Router const &) = delete;
  Router &operator=(Router &&) = delete;

  void Route(Handle handle, PacketPtr packet);

  void AddConnection(Handle handle);
  void RemoveConnection(Handle handle);

  /// @name Endpoint Methods (Publicly visible)
  /// @{
  void Send(Address const &address, uint16_t service, uint16_t channel,
            Payload const &message) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload) override;

  void Broadcast(uint16_t service, uint16_t channel, Payload const &payload) override;

  Response Exchange(Address const &address, uint16_t service, uint16_t channel,
                    Payload const &request) override;

  SubscriptionPtr Subscribe(uint16_t service, uint16_t channel) override;
  SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel) override;

  RoutingTable GetRoutingTable() const;
  /// @}

  void Cleanup();

  void Blacklist(Address const &target);
  void Whitelist(Address const &target);
  bool IsBlacklisted(Address const &target) const;
private:
  using HandleMap  = std::unordered_map<Handle, std::unordered_set<Packet::RawAddress>>;
  using Mutex      = mutex::Mutex;
  using Clock      = std::chrono::steady_clock;
  using Timepoint  = Clock::time_point;
  using EchoCache  = std::unordered_map<std::size_t, Timepoint>;
  using RawAddress = Packet::RawAddress;
  using BlackList  = fetch::muddle::Blacklist;

  bool AssociateHandleWithAddress(Handle handle, Packet::RawAddress const &address, bool direct);
  void DropPeer(Address const &peer);

  Handle LookupHandle(Packet::RawAddress const &address) const;
  Handle LookupRandomHandle(Packet::RawAddress const &address) const;

  void SendToConnection(Handle handle, PacketPtr packet);
  void RoutePacket(PacketPtr packet, bool external = true);
  void DispatchDirect(Handle handle, PacketPtr packet);
  void KillConnection(Handle handle);

  void DispatchPacket(PacketPtr packet);

  bool IsEcho(Packet const &packet, bool register_echo = true);
  void CleanEchoCache();

  Address const         address_;
  RawAddress const      address_raw_;
  MuddleRegister const &register_;
  BlackList             blacklist_;
  Dispatcher &          dispatcher_;
  SubscriptionRegistrar registrar_;

  mutable Mutex routing_table_lock_{__LINE__, __FILE__};
  RoutingTable  routing_table_;  ///< The map routing table from address to handle (Protected by
                                 ///< routing_table_lock_)
  HandleMap
      routing_table_handles_;  ///< The map of handles to address (Protected by routing_table_lock_)

  mutable Mutex echo_cache_lock_{__LINE__, __FILE__};
  EchoCache     echo_cache_;

  ThreadPool dispatch_thread_pool_;
};

}  // namespace muddle
}  // namespace fetch
