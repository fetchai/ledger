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

#include "core/byte_array/const_byte_array.hpp"
#include "core/periodic_runnable.hpp"
#include "core/reactor.hpp"
#include "core/service_ids.hpp"
#include "kademlia/address_priority.hpp"
#include "kademlia/peer_tracker_protocol.hpp"
#include "kademlia/table.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/tracker_configuration.hpp"
#include "muddle_register.hpp"
#include "peer_list.hpp"
#include "promise_runnable.hpp"

#include <chrono>
#include <memory>
#include <queue>
#include <vector>

namespace fetch {
namespace muddle {

class Muddle;
class PeerTracker : public core::PeriodicRunnable
{
public:
  using Clock                  = std::chrono::steady_clock;
  using TimePoint              = Clock::time_point;
  using Address                = Packet::Address;
  using Peers                  = std::deque<PeerInfo>;
  using PeerTrackerPtr         = std::shared_ptr<PeerTracker>;
  using PeerList               = std::unordered_set<Address>;
  using PendingPortResolution  = std::unordered_map<Address, std::shared_ptr<PromiseTask>>;
  using PendingPromised        = std::unordered_map<uint64_t, std::shared_ptr<PromiseTask>>;
  using ConnectionHandle       = network::AbstractConnection::ConnectionHandleType;
  using ConstByteArray         = byte_array::ConstByteArray;
  using PortsList              = PeerTrackerProtocol::PortsList;
  using ConnectionPriorityMap  = std::unordered_map<Address, AddressPriority>;
  using ConnectionPriorityList = std::vector<AddressPriority>;
  using AddressSet             = std::unordered_set<Address>;
  using AddressMap             = std::unordered_map<Address, Address>;
  using AddressTimestamp       = std::unordered_map<Address, TimePoint>;
  using PeerInfoList           = std::deque<PeerInfo>;

  struct UnresolvedConnection
  {
    ConnectionHandle handle;
    Address          address{};
    std::string      partial_uri{};
    PortsList        ports{};
  };

  enum ConnectionState
  {
    WAITING  = 1,
    RESOLVED = 2,
    DEAD     = 3
  };

  static PeerTrackerPtr New(Duration const &interval, core::Reactor &reactor,
                            MuddleRegister const &reg, PeerConnectionList &connections,
                            MuddleEndpoint &endpoint);

  /// Tracker interface
  /// @{
  AddressSet GetDesiredPeers() const;
  void       AddDesiredPeer(Address const &address);
  void       AddDesiredPeer(Address const &address, network::Peer const &hint);
  void       RemoveDesiredPeer(Address const &address);
  // TODO: Address GetRoutingAddress(Address const& destination);
  /// @}

  /// Trust interface
  /// @{
  void Blacklist(Address const & /*target*/);
  void Whitelist(Address const & /*target*/);
  bool IsBlacklisted(Address const & /*target*/) const;
  // TODO: void ReportBehaviour(Address,  double)
  /// @}

  /// Periodic maintainance
  /// @{
  void Periodically() override;
  /// @}

  /// Monitoring
  /// @{
  ConnectionPriorityMap connection_priority() const;
  std::size_t           known_peer_count() const;
  std::size_t           active_buckets() const;
  std::size_t           first_non_empty_bucket() const;
  AddressSet            keep_connections() const;
  AddressSet            longrange_connections() const;
  AddressSet            no_uri() const;
  AddressSet            incoming() const;
  AddressSet            outgoing() const;
  AddressSet            all_peers() const;
  AddressSet            desired_peers() const;
  /// @}

protected:
  friend class Muddle;
  /// Methods integrate new connections into the peer tracker.
  /// @{
  void AddConnectionHandleToQueue(ConnectionHandle handle);
  void SetMuddlePorts(PortsList const &ports);
  void SetConfiguration(TrackerConfiguration const &config);
  /// @}

private:
  PeerTracker(Duration const &interval, core::Reactor &reactor, MuddleRegister const &reg,
              PeerConnectionList &connections, MuddleEndpoint &endpoint);

  /// Connectivity maintenance
  /// @{
  void ConnectToDesiredPeers();
  void UpdatePriorityList(ConnectionPriorityMap & connection_priority,
                          ConnectionPriorityList &prioritized_peers, Peers const &peers);
  void ConnectToPeers(AddressSet &connections_made, ConnectionPriorityList const &prioritized_peers,
                      uint64_t const &max_connections);
  void DisconnectDuplicates();
  void DisconnectFromPeers();
  /// @}

  /// Pipeline for registering details of incoming and outgoing
  /// connections
  /// @{
  void            ProcessConnectionHandles();
  ConnectionState ResolveConnectionDetails(UnresolvedConnection &details);
  void            OnResolvePorts(UnresolvedConnection details, service::Promise const &promise);
  void            RegisterConnectionDetails(UnresolvedConnection const &details);
  /// @}

  /// Network discovery
  /// @{
  void PullPeerKnowledge();
  void SchedulePull(Address const &address);
  void SchedulePull(Address const &address, Address const &search_for);
  void OnResolvedPull(uint64_t pull_id, Address peer, Address search_for,
                      service::Promise const &promise);
  /// @}

  /// Thread-safety
  /// @{
  mutable std::mutex mutex_;
  /// @}

  /// Core components for maintaining connectivity.
  /// @{
  core::Reactor &       reactor_;
  MuddleRegister const &register_;
  MuddleEndpoint &      endpoint_;
  PeerConnectionList &  connections_;
  KademliaTable         peer_table_;
  Address               own_address_;
  /// @}

  /// Peer tracker protocol
  /// @{
  rpc::Client          rpc_client_;
  rpc::Server          rpc_server_;
  PeerTrackerProtocol  peer_tracker_protocol_;
  TrackerConfiguration tracker_configuration_;
  AddressSet           keep_connections_{};
  /// @}

  /// User defined connections
  /// @{
  AddressSet desired_peers_;
  /// @}

  /// Handling new comers
  /// @{
  std::queue<UnresolvedConnection> new_handles_;
  PendingPortResolution            port_resolution_promises_;
  /// @}

  /// Managing connections to Kademlia subtrees
  /// @{
  ConnectionPriorityMap  kademlia_connection_priority_;
  ConnectionPriorityList kademlia_prioritized_peers_;
  AddressSet             kademlia_connections_;
  //  uint64_t               persistent_outgoing_connections_{0};  /// TODO: rename?

  /// @}

  /// Managing connections accross subtrees.
  /// @{
  ConnectionPriorityMap  longrange_connection_priority_;
  ConnectionPriorityList longrange_prioritized_peers_;
  AddressSet             longrange_connections_;
  //  uint64_t               persistent_longrange_connections_{0};  /// TODO: rename?
  /// @}

  /// Management variables for network discovery
  /// @{
  std::deque<Address>                    peer_pull_queue_;
  AddressMap                             peer_pull_map_;
  PendingPromised                        pull_promises_;
  std::unordered_map<Address, TimePoint> last_pull_from_peer_;
  uint64_t                               pull_next_id_{0};
  /// @}

  /// Logging sets
  /// @{
  AddressSet no_uri_{};  ///< TODO:  Get rid of it?
  /// @}

  std::string logging_name_{"not-set"};
};  // namespace muddle

}  // namespace muddle
}  // namespace fetch
