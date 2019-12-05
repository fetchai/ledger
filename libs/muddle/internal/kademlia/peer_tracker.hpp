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
#include "core/byte_array/const_byte_array.hpp"
#include "core/periodic_runnable.hpp"
#include "core/reactor.hpp"
#include "core/service_ids.hpp"
#include "kademlia/address_priority.hpp"
#include "kademlia/peer_tracker_protocol.hpp"
#include "kademlia/table.hpp"
#include "moment/clock_interfaces.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/tracker_configuration.hpp"
#include "muddle_register.hpp"
#include "peer_list.hpp"
#include "promise_runnable.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <random>
#include <vector>

namespace fetch {
namespace muddle {

class Muddle;
class PeerTracker : public core::PeriodicRunnable
{
public:
  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

  using Address           = Packet::Address;
  using Peers             = std::deque<PeerInfo>;
  using PeerTrackerPtr    = std::shared_ptr<PeerTracker>;
  using PeerList          = std::unordered_set<Address>;
  using PendingResolution = std::unordered_map<Address, std::shared_ptr<PromiseTask>>;
  using PendingPromised   = std::unordered_map<uint64_t, std::shared_ptr<PromiseTask>>;
  using ConnectionHandle  = network::AbstractConnection::ConnectionHandleType;
  using ConstByteArray    = byte_array::ConstByteArray;

  using Ports                  = PeerTrackerProtocol::Ports;
  using ConnectionPriorityMap  = std::unordered_map<Address, AddressPriority>;
  using ConnectionPriorityList = std::vector<AddressPriority>;
  using AddressSet             = std::unordered_set<Address>;
  using AddressMap             = std::unordered_map<Address, Address>;
  using AddressTimestamp       = std::unordered_map<Address, Timepoint>;
  using PeerInfoList           = std::deque<PeerInfo>;
  using BlackList              = fetch::muddle::Blacklist;
  using Uri                    = network::Uri;
  using NetworkUris            = std::vector<Uri>;
  using Handle                 = network::AbstractConnection::ConnectionHandleType;
  using AddressToHandles       = std::unordered_map<Address, std::unordered_set<Handle>>;

  struct UnresolvedConnection
  {
    ConnectionHandle handle;
    Address          address{};
    std::string      partial_uri{};
    NetworkUris      uris{};
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
  ~PeerTracker()
  {
    Stop();
  }

  /// Tracker interface
  /// @{
  AddressSet GetDesiredPeers() const;
  void       AddDesiredPeer(Address const & address,
                            Duration const &expiry = muddle::MuddleInterface::NeverExpire());
  void       AddDesiredPeer(Address const &address, network::Peer const &hint,
                            Duration const &expiry =
                                muddle::MuddleInterface::NeverExpire());  // TODO(tfr): change hint to URI
  void       AddDesiredPeer(Uri const &     uri,
                            Duration const &expiry = muddle::MuddleInterface::NeverExpire());
  void       RemoveDesiredPeer(Address const &address);
  /// @}

  /// Low-level
  /// @{
  Handle LookupHandle(Address const &address) const
  {
    auto wptr = register_.LookupConnection(address);

    // If it is a direct connection we just return the handle
    auto connection = wptr.lock();
    if (connection)
    {
      return connection->handle();
    }

    // TODO(tfr): Create a cache for the search below

    // Finding best address
    Address          best_address{};
    KademliaDistance best = MaxKademliaDistance();

    {
      FETCH_LOCK(direct_mutex_);

      // Finding best address
      auto target_kad = KademliaAddress::Create(address);
      for (auto &peer : directly_connected_peers_)
      {
        KademliaAddress cmp  = KademliaAddress::Create(peer);
        auto            dist = GetKademliaDistance(target_kad, cmp);

        if (dist < best)
        {
          best         = dist;
          best_address = peer;
        }
      }

      // Comparing against own address
      auto own_kad = KademliaAddress::Create(own_address_);
      auto dist    = GetKademliaDistance(target_kad, own_kad);

      // In case the current node has a shorter distance, we return
      // 0 to indicate that the packet should not move
      if (dist < best)
      {
        return 0;
      }
    }

    // Finding handle
    wptr       = register_.LookupConnection(best_address);
    connection = wptr.lock();
    if (connection)
    {
      // TODO(tfr): add to cache
      return connection->handle();
    }

    return 0;
  }

  Handle LookupRandomHandle() const
  {
    FETCH_LOCK(direct_mutex_);
    std::vector<Address> all_addresses{directly_connected_peers_.begin(),
                                       directly_connected_peers_.end()};

    thread_local std::random_device rd;
    thread_local std::mt19937       g(rd());
    std::shuffle(all_addresses.begin(), all_addresses.end(), g);

    while (!all_addresses.empty())
    {
      auto const address = all_addresses.back();
      all_addresses.pop_back();

      auto wptr = register_.LookupConnection(address);

      auto connection = wptr.lock();
      if (connection)
      {
        return connection->handle();
      }
    }

    return 0;
  }
  /// @}

  /// Trust interface
  /// @{
  void Blacklist(Address const & /*target*/);
  void Whitelist(Address const & /*target*/);
  bool IsBlacklisted(Address const & /*target*/) const;
  // TODO(tfr): void ReportBehaviour(Address,  double)
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
  AddressSet            directly_connected_peers() const
  {
    FETCH_LOCK(direct_mutex_);
    return directly_connected_peers_;
  }
  /// @}

protected:
  friend class Muddle;
  /// Methods integrate new connections into the peer tracker.
  /// @{
  void AddConnectionHandle(ConnectionHandle handle);
  void RemoveConnectionHandle(ConnectionHandle handle);
  void UpdateExternalUris(NetworkUris const &uris);
  void UpdateExternalPorts(Ports const &ports);
  void SetConfiguration(TrackerConfiguration const &config);
  void Stop()
  {
    FETCH_LOG_WARN(logging_name_.c_str(), "Stopping peer tracker.");
    FETCH_LOCK(mutex_);
    stopping_              = true;
    tracker_configuration_ = TrackerConfiguration::AllOff();

    connection_expiry_.clear();
    desired_uri_expiry_.clear();
    desired_peers_.clear();
    kademlia_connection_priority_.clear();
    kademlia_prioritized_peers_.clear();
    kademlia_connections_.clear();
    longrange_connection_priority_.clear();
    longrange_prioritized_peers_.clear();
    longrange_connections_.clear();
  }
  /// @}

private:
  explicit PeerTracker(Duration const &interval, core::Reactor &reactor, MuddleRegister const &reg,
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
  void DisconnectFromSelf();
  /// @}

  /// Pipeline for registering details of incoming and outgoing
  /// connections
  /// @{
  void            ProcessConnectionHandles();
  ConnectionState ResolveConnectionDetails(UnresolvedConnection &details);
  void            OnResolveUris(UnresolvedConnection details, service::Promise const &promise);
  void            RegisterConnectionDetails(UnresolvedConnection const &details);
  /// @}

  /// Network discovery
  /// @{
  void PullPeerKnowledge();
  void SchedulePull(Address const &address);
  void SchedulePull(Address const &address, Address const &search_for);
  void OnResolvedPull(uint64_t pull_id, Address const &peer, Address const &search_for,
                      service::Promise const &promise);
  /// @}

  /// Thread-safety
  /// @{
  mutable std::mutex mutex_;
  mutable std::mutex direct_mutex_;   ///< Use to protect directly connected
                                      /// peers to avoid causing a deadlock
  mutable std::mutex desired_mutex_;  ///< Use to protect desired peer variables
                                      /// peers to avoid causing a deadlock
  /// @}

  /// Core components for maintaining connectivity.
  /// @{
  std::atomic<bool>     stopping_{false};
  core::Reactor &       reactor_;
  MuddleRegister const &register_;
  MuddleEndpoint &      endpoint_;
  PeerConnectionList &  connections_;
  KademliaTable         peer_table_;
  Address               own_address_;
  BlackList             blacklist_;
  /// @}

  /// Peer tracker protocol
  /// @{
  rpc::Client          rpc_client_;
  rpc::Server          rpc_server_;
  PeerTrackerProtocol  peer_tracker_protocol_;
  TrackerConfiguration tracker_configuration_;
  AddressSet           keep_connections_{};
  AddressSet           directly_connected_peers_{};
  /// @}

  /// User defined connections
  /// @{
  std::unordered_map<Address, Timepoint> connection_expiry_;
  std::unordered_map<Uri, Timepoint>     desired_uri_expiry_;
  AddressSet                             desired_peers_;
  std::unordered_set<Uri>                desired_uris_;
  /// @}

  /// Handling new comers
  /// @{
  std::queue<UnresolvedConnection> new_handles_;
  PendingResolution                uri_resolution_tasks_;
  /// @}

  /// Managing connections to Kademlia subtrees
  /// @{
  ConnectionPriorityMap  kademlia_connection_priority_;
  ConnectionPriorityList kademlia_prioritized_peers_;
  AddressSet             kademlia_connections_;
  /// @}

  /// Managing connections accross subtrees.
  /// @{
  ConnectionPriorityMap  longrange_connection_priority_;
  ConnectionPriorityList longrange_prioritized_peers_;
  AddressSet             longrange_connections_;
  /// @}

  /// Management variables for network discovery
  /// @{
  std::deque<Address>                    peer_pull_queue_;
  AddressMap                             peer_pull_map_;
  PendingPromised                        pull_promises_;
  std::unordered_map<Address, Timepoint> last_pull_from_peer_;
  uint64_t                               pull_next_id_{0};
  /// @}

  /// Logging sets
  /// @{
  AddressSet no_uri_{};  ///< TODO(tfr):  Get rid of it?
  /// @}

  std::string logging_name_{"not-set"};
};  // namespace muddle

}  // namespace muddle
}  // namespace fetch
