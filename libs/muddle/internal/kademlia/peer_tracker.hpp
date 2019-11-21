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

class PeerTracker : public core::PeriodicRunnable
{
public:
  struct TrackingTask;
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
  using AddressTimestamp       = std::unordered_map<Address, TimePoint>;
  using PeerInfoList           = std::deque<PeerInfo>;
  using TrackingTasks          = std::unordered_map<Address, TrackingTask>;
  using TrackingCallback       = std::function<void(Address, std::string)>;

  struct UnresolvedConnection
  {
    ConnectionHandle handle;
    Address          address{};
    std::string      partial_uri{};
    PortsList        ports{};
    bool             is_incoming{false};
  };

  struct TrackingTask
  {
    enum Priority
    {
      LOW    = 0,
      NORMAL = 1,
      HIGH   = 2
    };

    Priority         priority;
    Address          address{};
    PeerInfoList     next_peers{};
    AddressSet       already_visited{};
    TrackingCallback on_found{};
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
  /// Message delivery
  /// @{
  // void NotifyOnFindPeer(Address address, PeerConnectedCallback cb);
  /// @}

  /// Trust interface
  /// @{
  // From Muddle
  void Blacklist(Address const & /*target*/);
  void Whitelist(Address const & /*target*/);
  bool IsBlacklisted(Address const & /*target*/) const;
  /// @}

  /// Periodic maintainance
  /// @{
  void Periodically() override;
  /// @}

  /// Methods integrate new connections into the peer tracker.
  /// @{
  void AddConnectionHandleToQueue(ConnectionHandle handle);
  void SetMuddlePorts(PortsList const &ports);
  /// @}

  ConnectionPriorityMap connection_priority() const;

private:
  PeerTracker(Duration const &interval, core::Reactor &reactor, MuddleRegister const &reg,
              PeerConnectionList &connections, MuddleEndpoint &endpoint);

  /// Connectivity maintenance
  /// @{
  void UpdatePriorityList();
  void AddMostPromisingPeers();
  /// @}

  /// Pipeline for tracking peers
  /// @{
  void            Track(Address address_to_track);
  void            OnResolvedTracking(uint64_t tracker_id, Address peer, Address address_to_track,
                                     service::Promise const &promise);
  ConnectionState ResolveConnectionDetails(UnresolvedConnection &details);
  /// @}

  /// Pipeline for registering details of incoming and outgoing
  /// connections
  /// @{
  void ProcessConnectionHandles();
  void OnResolvePorts(UnresolvedConnection details, service::Promise const &promise);
  void RegisterConnectionDetails(UnresolvedConnection const &details);
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

  /// Peer tracker protocol
  /// @{
  rpc::Client          rpc_client_;
  rpc::Server          rpc_server_;
  PeerTrackerProtocol  peer_tracker_protocol_;
  uint64_t             track_cycle_counter_{0};
  TrackerConfiguration tracker_configuration_;
  /// @}

  std::queue<UnresolvedConnection> new_handles_;
  PendingPortResolution            port_resolution_promises_;

  /// Tracking
  /// @{
  uint64_t        next_tracker_id_{0};
  PendingPromised tracking_promises_;
  TrackingTasks   tracking_tasks_;
  /// @}

  /// Maintenance cycle
  /// @{
  Address                own_address_;             ///< Own address
  ConnectionPriorityMap  connection_priority_;     ///< Contains all relevant address priorities.
  ConnectionPriorityList prioritized_peers_;       ///< List with all address in priority.
  uint64_t   persistent_outgoing_connections_{0};  ///< Number of persistent outgoing connections
  uint64_t   shortlived_outgoing_connections_{0};  ///< Number of shortlived outgoing connections
  AddressSet keep_connections_{};                  ///< Addresses that should not be disconnected
  ConnectionPriorityMap next_priority_map_{};      ///< Trimmed priority map
  /// @}
};  // namespace muddle

}  // namespace muddle
}  // namespace fetch