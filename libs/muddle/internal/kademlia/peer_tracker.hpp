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
  using PeerTrackerPtr        = std::shared_ptr<PeerTracker>;
  using PeerList              = std::unordered_set<Address>;
  using PendingPortResolution = std::unordered_map<Address, std::shared_ptr<PromiseTask>>;
  using PendingPromised       = std::unordered_map<uint64_t, std::shared_ptr<PromiseTask>>;
  using ConnectionHandle      = network::AbstractConnection::ConnectionHandleType;
  using ConstByteArray        = byte_array::ConstByteArray;
  using PortsList             = PeerTrackerProtocol::PortsList;
  using ConnectionPriorityMap = std::unordered_map<Address, AddressPriority>;

  struct UnresolvedConnection
  {
    ConnectionHandle handle;
    Address          address{};
    std::string      partial_uri{};
    PortsList        ports{};
    bool             is_incoming{false};
  };

  enum ConnectionState
  {
    WAITING  = 1,
    RESOLVED = 2,
    DEAD     = 3
  };

  static PeerTrackerPtr New(Duration const &interval, core::Reactor &reactor,
                            MuddleRegister const &reg, PeerConnectionList &connections,
                            MuddleEndpoint &endpoint)
  {
    PeerTrackerPtr ret;
    ret.reset(new PeerTracker(interval, reactor, reg, connections, endpoint));
    return ret;
  }

  /// Message delivery
  /// @{
  // void NotifyOnFindPeer(Address address, PeerConnectedCallback cb);
  /// @}

  /// Trust interface
  /// @{
  // From Muddle
  void Blacklist(Address const & /*target*/)
  {}
  void Whitelist(Address const & /*target*/)
  {}
  bool IsBlacklisted(Address const & /*target*/) const
  {
    return false;
  }
  /// @}

  void AddConnectionHandleToQueue(ConnectionHandle handle)
  {
    FETCH_LOCK(mutex_);
    UnresolvedConnection conn_details;
    conn_details.handle = handle;
    new_handles_.push(conn_details);
  }

  /// Periodic maintainance
  /// @{
  void Periodically() override
  {
    FETCH_LOCK(mutex_);
    return;

    // TODO: Calculate as in peerselector
    auto       timeout = std::chrono::duration_cast<PromiseTask::Duration>(std::chrono::seconds{1});
    auto const currently_connected_peers = register_.GetCurrentAddressSet();
    auto const currently_incoming        = register_.GetIncomingAddressSet();

    // Resolving all handles
    std::queue<UnresolvedConnection> not_resolved;
    while (!new_handles_.empty())
    {
      auto details = new_handles_.front();
      new_handles_.pop();

      // Waiting for details to be resolved
      auto state = ResolveConnectionDetails(details);
      if (state == ConnectionState::WAITING)
      {
        not_resolved.push(details);
      }
      else if (state == ConnectionState::RESOLVED)
      {
        // If not ports were detected, we request the client for its server ports
        if (details.ports.empty())
        {
          // make the call to the remote service
          auto promise = rpc_client_.CallSpecificAddress(details.address, RPC_MUDDLE_KADEMLIA,
                                                         PeerTrackerProtocol::GET_MUDDLE_PORTS);

          // wrap the promise is a task
          auto task = std::make_shared<PromiseTask>(
              promise, timeout, [this, details](service::Promise const &promise) {
                OnResolvePorts(details, promise);
              });

          // add the task to the reactor
          reactor_.Attach(task);

          // add the task to the pending resolutions queue
          port_resolution_promises_.emplace(details.address, std::move(task));
        }
      }
    }
    std::swap(new_handles_, not_resolved);

    // Tracking own address will result in finding neighbourhood peers
    // as own address is not contained in table

    // TODO: Add tracking time
    if (track_cycle_counter_ % 32 == 0)
    {
      Track(own_address_);
    }
    ++track_cycle_counter_;

    // Updating the priority of the peer list.
    std::vector<AddressPriority> prioritized_peers;
    for (auto &p : connection_priority_)
    {
      p.second.UpdatePriority();
      prioritized_peers.push_back(p.second);
      // TODO: Skip own
    }
    //
    // Sorting according to priority
    std::sort(prioritized_peers.begin(), prioritized_peers.end(),
              [](auto const &a, auto const &b) -> bool {
                // Making highest priority appear first
                return a.priority > b.priority;
              });

    // TODO: Trim to a maximum nuber of permanent connections

    // Connecting - note that the order matters as we connect to the closest nodes first
    ConnectionPriorityMap       new_priority_map;
    uint64_t                    persistent_outgoing_connections{0};
    uint64_t                    shortlived_outgoing_connections{0};
    std::unordered_set<Address> keep_connections;
    uint64_t                    n = 0;

    for (; n < prioritized_peers.size(); ++n)
    {
      if (persistent_outgoing_connections > 1)  // TODO: Fix parameter
      {
        break;
      }

      auto &p = prioritized_peers[n];

      if (p.address == own_address_)
      {
        continue;
      }

      // Ignoring addresses found in incoming
      if (currently_incoming.find(p.address) != currently_incoming.end())
      {
        continue;
      }

      // We allow up to 2 short lived connections
      if (shortlived_outgoing_connections > 1)
      {
        if (!p.persistent)
        {
          continue;
        }
      }

      if (p.is_incoming)
      {
        // We skip all incoming connections to avoid bidirectional connectivity
        continue;
      }

      // TODO: Check if we should convert the connection to persistent
      // If not connected, we connect
      if (currently_connected_peers.find(p.address) == currently_connected_peers.end())
      {
        auto suri = peer_table_.GetUri(p.address);
        // If we are not connected, we connect.
        network::Uri uri;
        if (suri.empty())
        {
          continue;
        }

        uri.Parse(suri);
        connections_.AddPersistentPeer(uri);
        p.is_connected = true;
        p.is_incoming  = false;
      }

      // TODO: Test that connection can be found

      // Upgrading connection
      if (p.PreferablyPersistent())
      {
        p.MakePersistent();
      }

      if (p.persistent)
      {

        ++persistent_outgoing_connections;
      }
      else
      {
        ++shortlived_outgoing_connections;
      }

      keep_connections.insert(p.address);
      new_priority_map.insert({p.address, p});
    }

    // Maintenaining short lived connections
    for (; n < prioritized_peers.size(); ++n)
    {

      if (shortlived_outgoing_connections > 1)  // TODO: Fix parameter
      {
        break;
      }

      auto &p = prioritized_peers[n];

      if (p.address == own_address_)
      {
        continue;
      }

      // Ignoring addresses found in incoming
      if (currently_incoming.find(p.address) != currently_incoming.end())
      {
        continue;
      }

      // We stop when the priority becomes too low
      if (p.priority < 0.01)
      {
        break;
      }

      if (p.is_incoming)  // TODO: Refactor such that the priority list is only for outgoing
      {
        // We skip all incoming connections to avoid bidirectional connectivity
        continue;
      }

      // If not connected, we connect
      if (currently_connected_peers.find(p.address) == currently_connected_peers.end())
      {
        auto suri = peer_table_.GetUri(p.address);
        // If we are not connected, we connect.
        network::Uri uri;
        if (suri.empty())
        {
          continue;
        }

        uri.Parse(suri);
        connections_.AddPersistentPeer(uri);
        p.is_connected = true;
        p.is_incoming  = false;
      }

      p.MakeTemporary();

      keep_connections.insert(p.address);
      new_priority_map.insert({p.address, p});
      ++shortlived_outgoing_connections;
    }

    // Managing terminating connections that are not needed.
    auto connecting_to = register_.GetOutgoingAddressSet();

    // Disconnecting from the remaining  nodes
    for (auto const &address : connecting_to)
    {
      if (keep_connections.find(address) != keep_connections.end())
      {
        continue;
      }

      auto conn = register_.LookupConnection(address).lock();
      if (conn)
      {
        FETCH_LOG_WARN("TODO2:logging_name_", "Dropping Address: ", address.ToBase64());

        auto const handle = conn->handle();

        connections_.RemoveConnection(handle);
        connections_.RemovePersistentPeer(handle);
      }
    }

    // Setting the new priority list
    std::cout << "Connection stats: " << persistent_outgoing_connections << " "
              << shortlived_outgoing_connections << " " << new_priority_map.size() << std::endl;
    std::swap(connection_priority_, new_priority_map);
  }
  /// @}

  void SetMuddlePorts(PortsList const &ports)
  {
    peer_tracker_protocol_.SetMuddlePorts(ports);
  }

  ConnectionPriorityMap connection_priority() const
  {
    FETCH_LOCK(mutex_);
    return connection_priority_;
  }

private:
  PeerTracker(Duration const &interval, core::Reactor &reactor, MuddleRegister const &reg,
              PeerConnectionList &connections, MuddleEndpoint &endpoint)
    : core::PeriodicRunnable(interval)
    , reactor_{reactor}
    , register_{reg}
    , connections_{connections}
    , endpoint_{endpoint}
    , peer_table_{endpoint_.GetAddress()}
    , rpc_client_{"PeerTracker", endpoint_, SERVICE_MUDDLE_PEER_TRACKER, CHANNEL_RPC}
    , rpc_server_(endpoint_, SERVICE_MUDDLE_PEER_TRACKER, CHANNEL_RPC)
    , peer_tracker_protocol_{peer_table_}
    , own_address_{endpoint_.GetAddress()}
  {
    rpc_server_.Add(RPC_MUDDLE_KADEMLIA, &peer_tracker_protocol_);
  }

  void Track(Address address_to_track)
  {
    // Finding initial peers to track from
    auto       direct_peers              = endpoint_.GetDirectlyConnectedPeerSet();
    auto       peers                     = peer_table_.FindPeer(address_to_track);
    auto const currently_connected_peers = register_.GetCurrentAddressSet();

    // Adding new peers to priority list
    for (auto &p : peers)
    {
      if (currently_connected_peers.find(p.address) == currently_connected_peers.end())
      {
        AddressPriority priority;
        priority.address      = p.address;
        priority.bucket       = GetBucket(p.distance);
        priority.is_connected = false;
        priority.persistent   = true;

        connection_priority_.insert({p.address, std::move(priority)});
      }
    }

    // TODO: Calculate as in peerselector
    auto timeout = std::chrono::duration_cast<PromiseTask::Duration>(std::chrono::seconds{1});

    // TODO: make 3 a parameter for the tracker.
    uint64_t N = std::min(static_cast<unsigned long>(3), peers.size());

    // tracking the peer through the N closest peers
    uint64_t j = 0;
    for (uint64_t i = 0; (j < N) && (i < peers.size()); ++i)
    {
      // We only track through directly connected peers
      // TODO: Will fail if all found addresses are unconnected
      auto &peer = peers[i];
      if (direct_peers.find(peer.address) == direct_peers.end())
      {
        continue;
      }
      ++j;

      auto tracker_id = next_tracker_id_++;

      // make the call to the remote service
      // TODO: Make sure that this only uses direct connections
      auto promise = rpc_client_.CallSpecificAddress(
          peer.address, RPC_MUDDLE_KADEMLIA, PeerTrackerProtocol::FIND_PEERS, address_to_track);

      // wrap the promise is a task
      auto task = std::make_shared<PromiseTask>(
          promise, timeout,
          [this, peer, address_to_track, tracker_id](service::Promise const &promise) {
            OnResolvedTracking(tracker_id, peer.address, address_to_track, promise);
          });

      // add the task to the reactor
      reactor_.Attach(task);

      // add the task to the pending resolutions queue
      tracking_promises_.emplace(tracker_id, std::move(task));
    }
  }

  void OnResolvedTracking(uint64_t tracker_id, Address peer, Address address_to_track,
                          service::Promise const &promise)
  {
    FETCH_LOCK(mutex_);
    if (promise->state() == service::PromiseState::SUCCESS)
    {
      // Reporting that peer is still responding
      peer_table_.ReportLiveliness(peer);

      // extract the set of addresses from which the prospective node is contactable
      auto peer_info_list = promise->As<std::deque<PeerInfo>>();

      // create the new entry and populate
      for (auto const &peer_info : peer_info_list)
      {
        // reporting the possible existence of another peer on the network.
        peer_table_.ReportLiveliness(peer_info.address, peer_info);
      }

      // Scheduling short lived connections in case the address was not found.
      if (peer != address_to_track)
      {
        for (auto const &peer_info : peer_info_list)
        {
          AddressPriority priority;
          priority.address      = peer_info.address;
          priority.is_connected = false;
          // TODO: Re-compute distance
          priority.bucket = GetBucket(peer_info.distance);

          priority.persistent  = false;
          priority.is_incoming = false;
          priority.desired_expiry =
              std::chrono::system_clock::now() + std::chrono::seconds(10);  // Expires in 10s
          connection_priority_.insert({peer_info.address, std::move(priority)});
        }
      }
    }
    else
    {
      FETCH_LOG_WARN("TODO:logging_name_", "Unable to resolve address for: ", peer.ToBase64(),
                     " code: ", int(promise->state()));

      // In case of failure, we report it to the tracker table.
      peer_table_.ReportFailure(peer);
    }

    // remove the entry from the pending list
    tracking_promises_.erase(tracker_id);
  }

  ConnectionState ResolveConnectionDetails(UnresolvedConnection &details)
  {
    auto connection = register_.LookupConnection(details.handle).lock();

    if (connection)
    {
      // We ignore connections for which we do not know the
      // address yet.
      if (connection->Address().empty())
      {
        return ConnectionState::WAITING;
      }

      // Getting the address key
      details.address = register_.GetAddress(details.handle);

      // If the address for the handle is empty, we reschedule
      // for later
      if (details.address.empty())
      {
        return ConnectionState::WAITING;
      }

      // Getting the network endpoint without port
      details.partial_uri = connection->Address();

      // We can only rely on ports for outgoing connections.
      if (connection->Type() == network::AbstractConnection::TYPE_OUTGOING)
      {
        details.is_incoming = false;
        details.ports       = PortsList({connection->port()});
        RegisterConnectionDetails(details);
      }
      else
      {
        details.is_incoming = true;
      }

      return ConnectionState::RESOLVED;
    }

    return ConnectionState::DEAD;
  }

  void OnResolvePorts(UnresolvedConnection details, service::Promise const &promise)
  {
    FETCH_LOCK(mutex_);
    if (promise->state() == service::PromiseState::SUCCESS)
    {
      // extract the set of addresses from which the prospective node is contactable
      auto ports = promise->As<PortsList>();

      details.ports = std::move(ports);
      RegisterConnectionDetails(details);
    }
  }

  void RegisterConnectionDetails(UnresolvedConnection const &details)
  {
    // Reporting that peer is still responding
    network::Peer peer(details.partial_uri, details.ports[0]);
    auto          uri = peer.ToUri();

    PeerInfo info;

    info.address = details.address;
    info.uri     = std::move(uri);

    peer_table_.ReportLiveliness(details.address, info);
    bool found = false;

    // Finding address priority
    for (auto &element : connection_priority_)
    {
      auto &address_priority = element.second;
      if (address_priority.address == details.address)
      {
        address_priority.is_connected = true;
        found                         = true;
      }
    }

    // If the address does not exist in the priorities we
    // create it.
    if (!found)
    {
      AddressPriority priority;
      priority.address      = details.address;
      priority.is_connected = false;

      // By default we assume that incoming connections
      // are not persistent
      if (details.is_incoming)
      {
        priority.persistent  = false;
        priority.is_incoming = true;
      }

      connection_priority_.insert({details.address, std::move(priority)});
    }
  }

  mutable std::mutex mutex_;

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
  rpc::Client         rpc_client_;
  rpc::Server         rpc_server_;
  PeerTrackerProtocol peer_tracker_protocol_;
  uint64_t            track_cycle_counter_{0};
  /// @}

  std::queue<UnresolvedConnection> new_handles_;
  PendingPortResolution            port_resolution_promises_;

  /// Connection management
  /// @{
  PendingPromised       tracking_promises_;
  uint64_t              next_tracker_id_{0};
  Address               own_address_;
  ConnectionPriorityMap connection_priority_;
  /// @}
};  // namespace muddle

}  // namespace muddle
}  // namespace fetch