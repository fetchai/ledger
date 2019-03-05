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

#include "core/mutex.hpp"
#include "crypto/prover.hpp"
#include "network/details/thread_pool.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/muddle/blacklist.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/network_id.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/subscription_registrar.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"

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
  using Address             = Packet::Address;  // == a crypto::Identity.identifier_
  using PacketPtr           = std::shared_ptr<Packet>;
  using Payload             = Packet::Payload;
  using ConnectionPtr       = std::weak_ptr<network::AbstractConnection>;
  using Handle              = network::AbstractConnection::connection_handle_type;
  using ThreadPool          = network::ThreadPool;
  using HandleDirectAddrMap = std::unordered_map<Handle, Address>;
  using Prover              = crypto::Prover;

  struct RoutingData
  {
    bool   direct = false;
    Handle handle = 0;
  };

  using RoutingTable = std::unordered_map<Packet::RawAddress, RoutingData>;

  static constexpr char const *LOGGING_NAME = "Router";

  // Helper functions
  static Packet::RawAddress ConvertAddress(Packet::Address const &address);
  static Packet::Address    ConvertAddress(Packet::RawAddress const &address);

  // Construction / Destruction
  Router(NetworkId network_id, Address address, MuddleRegister const &reg, Dispatcher &dispatcher, Prover *certificate = nullptr);
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

  AddressList GetDirectlyConnectedPeers() const override;

  RoutingTable GetRoutingTable() const;
  /// @}

  bool HandleToDirectAddress(const Handle &handle, Address &address) const;

  /** If this host is connected close their port.
   * @param peer The target address to killed.
   */
  void DropPeer(Address const &address);

  /** Kills the connection with the handle.
   * @param handle The handle to drop
   * @param address Supply the address to avoid an extra address lookup.
   */
  void DropHandle(Handle handle, const Address &address);

  void Cleanup();

  /** Show debugging information about the internals of the router.
   * @param prefix the string to put on the front of the logging lines.
   */
  void Debug(std::string const &prefix) const;

  /** Deny this host's connection attempts and do not attempt to connect to it.
   * @param target The target address to be denied.
   */
  void Blacklist(Address const &target);

  /** Allow this host to be connected to and to connect to us.
   * @param target The target address to be allowed.
   */
  void Whitelist(Address const &target);

  /** Return true if connections from this target address will be rejected.
   * @param target The target address's status to interrogate.
   */
  bool IsBlacklisted(Address const &target) const;

  /** Return true if there is an actual TCP connection to this address at the moment.
   * @param target The target address's status to interrogate.
   */
  bool IsConnected(Address const &target) const;

  /** Return the handle associated with an address or zero
   * @param target The target address's status to interrogate.
   * @returns A handle if one is available or zero.
   */
  Handle LookupHandleFromAddress(Packet::Address const &address) const;

  Handle LookupHandle(Packet::RawAddress const &address) const;

  // Operators
  Router &operator=(Router const &) = delete;
  Router &operator=(Router &&) = delete;

private:
  using HandleMap  = std::unordered_map<Handle, std::unordered_set<Packet::RawAddress>>;
  using Mutex      = mutex::Mutex;
  using Clock      = std::chrono::steady_clock;
  using Timepoint  = Clock::time_point;
  using EchoCache  = std::unordered_map<std::size_t, Timepoint>;
  using RawAddress = Packet::RawAddress;
  using BlackList  = fetch::muddle::Blacklist;

  static constexpr std::size_t NUMBER_OF_ROUTER_THREADS = 10;

  bool AssociateHandleWithAddress(Handle handle, Packet::RawAddress const &address, bool direct);

  Handle LookupRandomHandle(Packet::RawAddress const &address) const;

  void SendToConnection(Handle handle, PacketPtr packet);
  void RoutePacket(PacketPtr packet, bool external = true);
  void DispatchDirect(Handle handle, PacketPtr packet);
  void KillConnection(Handle handle, Address const &peer);
  void KillConnection(Handle handle);

  void DispatchPacket(PacketPtr packet, Address transmitter);

  bool IsEcho(Packet const &packet, bool register_echo = true);
  void CleanEchoCache();

  PacketPtr const &Sign(PacketPtr const &p) const;
  bool Genuine(PacketPtr const &p) const;

  Address const         address_;
  RawAddress const      address_raw_;
  MuddleRegister const &register_;
  BlackList             blacklist_;
  Dispatcher &          dispatcher_;
  SubscriptionRegistrar registrar_;
  NetworkId             network_id_;
  Prover               *prover_ = nullptr;

  mutable Mutex routing_table_lock_{__LINE__, __FILE__};
  RoutingTable  routing_table_;  ///< The map routing table from address to handle (Protected by
                                 ///< routing_table_lock_)
  HandleMap
      routing_table_handles_;  ///< The map of handles to address (Protected by routing_table_lock_)

  mutable Mutex echo_cache_lock_{__LINE__, __FILE__};
  EchoCache     echo_cache_;

  ThreadPool dispatch_thread_pool_;

  HandleDirectAddrMap direct_address_map_;  ///< Map of handles to direct address
  ///< (Protected by routing_table_lock)
};

}  // namespace muddle
}  // namespace fetch
