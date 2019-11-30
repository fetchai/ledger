#pragma once
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

  ///
  /// @{
  AddressSet GetDesiredPeers() const;
  void       AddDesiredPeer(Address const &address);
  void       AddDesiredPeer(Address const &address, network::Peer const &hint);
  void       RemoveDesiredPeer(Address const &address);
  /// @}

  /// Trust interface
  /// @{
  // From Muddle
  void Blacklist(Address const & /*target*/);
  void Whitelist(Address const & /*target*/);
  bool IsBlacklisted(Address const & /*target*/) const;
  // void ReportBehaviour(Address,  double)
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
  void UpdareLongLivedConnections(AddressSet &                  connections,
                                  ConnectionPriorityList const &prioritized_peers,
                                  uint64_t &connection_count, uint64_t const &max_connections);
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

  /// Thread-safety
  /// @{
  mutable std::mutex mutex_;
  /// @}

  /// Maintaining connectivity
  /// @{
  core::Reactor &       reactor_;
  MuddleRegister const &register_;
  MuddleEndpoint &      endpoint_;
  PeerConnectionList &  connections_;
  KademliaTable         peer_table_;
  /// @}

  /// User defined connections
  /// @{
  AddressSet desired_peers_;
  /// @}

  /// Peer tracker protocol
  /// @{
  rpc::Client          rpc_client_;
  rpc::Server          rpc_server_;
  PeerTrackerProtocol  peer_tracker_protocol_;
  TrackerConfiguration tracker_configuration_;
  /// @}

  std::queue<UnresolvedConnection> new_handles_;
  PendingPortResolution            port_resolution_promises_;

  ConnectionPriorityMap
                         kademlia_connection_priority_;  ///< Contains all relevant address priorities.
  ConnectionPriorityList kademlia_prioritized_peers_;  ///< List with all address in priority.
  AddressSet             kademlia_connections_;

  ConnectionPriorityMap
                         longrange_connection_priority_;  ///< Contains all relevant address priorities.
  ConnectionPriorityList longrange_prioritized_peers_;  ///< List with all address in priority.
  AddressSet             longrange_connections_;
  /// Maintenance cycle
  /// @{
  Address own_address_;  ///< Own address

  void PullPeerKnowledge();
  void SchedulePull(Address const &address);
  void SchedulePull(Address const &address, Address const &search_for);
  void OnResolvedPull(uint64_t pull_id, Address peer, Address search_for,
                      service::Promise const &promise);

  std::deque<Address>                    peer_pull_queue_;
  AddressMap                             peer_pull_map_;
  PendingPromised                        pull_promises_;
  std::unordered_map<Address, TimePoint> last_pull_from_peer_;
  uint64_t                               pull_next_id_{0};

  uint64_t persistent_outgoing_connections_{0};   ///< Number of persistent outgoing connections
  uint64_t persistent_longrange_connections_{0};  ///< Number of persistent outgoing connections
  /// @}

  /// Logging sets
  /// @{
  AddressSet            no_uri_{};
  AddressSet            keep_connections_{};   ///< Addresses that should not be disconnected
  ConnectionPriorityMap next_priority_map_{};  ///< Trimmed priority map
  /// @}

  std::string logging_name_{"not-set"};
};  // namespace muddle

}  // namespace muddle
}  // namespace fetch