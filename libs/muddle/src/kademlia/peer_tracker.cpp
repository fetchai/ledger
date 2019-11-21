#include "kademlia/peer_tracker.hpp"

#include <chrono>
#include <memory>
#include <queue>
#include <vector>

namespace fetch {
namespace muddle {

PeerTracker::PeerTrackerPtr PeerTracker::New(PeerTracker::Duration const &interval,
                                             core::Reactor &reactor, MuddleRegister const &reg,
                                             PeerConnectionList &connections,
                                             MuddleEndpoint &    endpoint)
{
  PeerTrackerPtr ret;
  ret.reset(new PeerTracker(interval, reactor, reg, connections, endpoint));
  return ret;
}

void PeerTracker::Blacklist(Address const & /*target*/)
{}

void PeerTracker::Whitelist(Address const & /*target*/)
{}

bool PeerTracker::IsBlacklisted(Address const & /*target*/) const
{
  return false;
}

void PeerTracker::AddConnectionHandleToQueue(ConnectionHandle handle)
{
  FETCH_LOCK(mutex_);
  UnresolvedConnection conn_details;
  conn_details.handle = handle;
  new_handles_.push(conn_details);
}

void PeerTracker::ProcessConnectionHandles()
{
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
            promise, tracker_configuration_.promise_timeout,
            [this, details](service::Promise const &promise) { OnResolvePorts(details, promise); });

        // add the task to the reactor
        reactor_.Attach(task);

        // add the task to the pending resolutions queue
        port_resolution_promises_.emplace(details.address, std::move(task));
      }
    }
  }
  std::swap(new_handles_, not_resolved);
}

void PeerTracker::UpdatePriorityList()
{
  for (auto &p : connection_priority_)
  {
    // Skipping own address
    if (p.first == own_address_)
    {
      continue;
    }

    // Updating and pushing to list
    p.second.UpdatePriority();
    prioritized_peers_.push_back(p.second);
  }

  // Sorting according to priority
  std::sort(prioritized_peers_.begin(), prioritized_peers_.end(),
            [](auto const &a, auto const &b) -> bool {
              // Making highest priority appear first
              return a.priority > b.priority;
            });
}

void PeerTracker::Periodically()
{
  FETCH_LOCK(mutex_);

  // Getting state of our connectivity
  auto const currently_connected_peers = register_.GetCurrentAddressSet();
  auto const currently_incoming        = register_.GetIncomingAddressSet();

  // Resolving all handles
  ProcessConnectionHandles();

  // Tracking own address will result in finding neighbourhood peers
  // as own address is not contained in table

  // TODO: Add tracking time
  if (track_cycle_counter_ % 32 == 0)
  {
    AddMostPromisingPeers();
    Track(own_address_);
  }
  ++track_cycle_counter_;

  // Updating the priority of the peer list.
  UpdatePriorityList();

  // Connecting - note that the order matters as we connect to the closest nodes first
  next_priority_map_.clear();
  persistent_outgoing_connections_ = 0;
  shortlived_outgoing_connections_ = 0;
  keep_connections_.clear();

  uint64_t n = 0;

  // Primary loop to ensure that we are conencted to high priority peers
  for (; n < prioritized_peers_.size(); ++n)
  {
    // If we have more longlived connections than the max,
    // we stop. The most important onces are now kept alive.
    if (persistent_outgoing_connections_ >= tracker_configuration_.max_longlived_outgoing)
    {
      break;
    }

    // Getting the peer details
    auto &p = prioritized_peers_[n];
    if (p.address == own_address_)
    {
      continue;
    }

    // Ignoring addresses found in incoming
    if (currently_incoming.find(p.address) != currently_incoming.end())
    {
      continue;
    }

    // Short lived connections might be high priority.
    // If we exceed the number of short lived connections, we skip
    // any there might be left.
    if (shortlived_outgoing_connections_ >= tracker_configuration_.max_shortlived_outgoing)
    {
      if (!p.persistent)
      {
        continue;
      }
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
      // TODO: add callback if failure on connection
      connections_.AddPersistentPeer(uri);
      p.is_connected = true;
      p.is_incoming  = false;
    }

    // Upgrading connection if we are better off
    // having the potentially shortlived connection as a
    // long lived one.
    if (p.PreferablyPersistent())
    {
      p.MakePersistent();
    }

    // Counting accordingly
    if (p.persistent)
    {
      ++persistent_outgoing_connections_;
    }
    else
    {
      ++shortlived_outgoing_connections_;
    }

    // Keeping track of what we have connected to.
    keep_connections_.insert(p.address);
    next_priority_map_.insert({p.address, p});
  }

  // In this loop we only add shortlived connections. Should we encountering
  // a previously longlived peer, we downgrade it.
  for (; n < prioritized_peers_.size(); ++n)
  {
    // Stopping at max number of shortlived connections
    if (shortlived_outgoing_connections_ >= tracker_configuration_.max_shortlived_outgoing)
    {
      break;
    }

    // Getting the peer
    auto &p = prioritized_peers_[n];
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

    // Making connection short-lived.
    p.MakeTemporary();

    // Keeping track of what we have added
    keep_connections_.insert(p.address);
    next_priority_map_.insert({p.address, p});
    ++shortlived_outgoing_connections_;
  }

  // Managing terminating connections that are not needed.
  auto connecting_to = register_.GetOutgoingAddressSet();

  // Disconnecting from the remaining  nodes
  for (auto const &address : connecting_to)
  {
    if (keep_connections_.find(address) != keep_connections_.end())
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
  std::cout << "Connection stats: " << persistent_outgoing_connections_ << " "
            << shortlived_outgoing_connections_ << " " << next_priority_map_.size() << std::endl;
  std::swap(connection_priority_, next_priority_map_);
}
/// @}

void PeerTracker::SetMuddlePorts(PortsList const &ports)
{
  peer_tracker_protocol_.SetMuddlePorts(ports);
}

PeerTracker::ConnectionPriorityMap PeerTracker::connection_priority() const
{
  FETCH_LOCK(mutex_);
  return connection_priority_;
}

PeerTracker::PeerTracker(PeerTracker::Duration const &interval, core::Reactor &reactor,
                         MuddleRegister const &reg, PeerConnectionList &connections,
                         MuddleEndpoint &endpoint)
  : core::PeriodicRunnable(interval)
  , reactor_{reactor}
  , register_{reg}
  , endpoint_{endpoint}
  , connections_{connections}
  , peer_table_{endpoint_.GetAddress()}
  , rpc_client_{"PeerTracker", endpoint_, SERVICE_MUDDLE_PEER_TRACKER, CHANNEL_RPC}
  , rpc_server_(endpoint_, SERVICE_MUDDLE_PEER_TRACKER, CHANNEL_RPC)
  , peer_tracker_protocol_{peer_table_}
  , own_address_{endpoint_.GetAddress()}
{
  rpc_server_.Add(RPC_MUDDLE_KADEMLIA, &peer_tracker_protocol_);
}

void PeerTracker::AddMostPromisingPeers()
{
  // Finding initial peers to track from
  // TODO: Make global variable since it is used many places
  auto const currently_connected_peers = register_.GetCurrentAddressSet();
  auto       peers                     = peer_table_.FindPeer(own_address_);

  // Adding new peers to priority list
  for (auto &p : peers)
  {
    // Only add the onces to which we are not already connected
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
}

void PeerTracker::Track(Address address_to_track)
{
  auto     direct_peers = endpoint_.GetDirectlyConnectedPeerSet();
  auto     peers        = peer_table_.FindPeer(address_to_track);
  uint64_t N =
      std::min(static_cast<unsigned long>(tracker_configuration_.async_calls), peers.size());

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
    auto promise = rpc_client_.CallSpecificAddress(
        peer.address, RPC_MUDDLE_KADEMLIA, PeerTrackerProtocol::FIND_PEERS, address_to_track);

    // wrap the promise is a task
    auto task = std::make_shared<PromiseTask>(
        promise, tracker_configuration_.promise_timeout,
        [this, peer, address_to_track, tracker_id](service::Promise const &promise) {
          OnResolvedTracking(tracker_id, peer.address, address_to_track, promise);
        });

    // add the task to the reactor
    reactor_.Attach(task);

    // add the task to the pending resolutions queue
    tracking_promises_.emplace(tracker_id, std::move(task));
  }
}

void PeerTracker::OnResolvedTracking(uint64_t tracker_id, Address peer, Address address_to_track,
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

        // TODO: Re-compute distance as you cannot trust any distance
        // given by others.
        priority.bucket = GetBucket(peer_info.distance);

        priority.persistent  = false;
        priority.is_incoming = false;
        priority.desired_expiry =
            AddressPriority::Clock::now() + tracker_configuration_.default_connection_expiry;
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

PeerTracker::ConnectionState PeerTracker::ResolveConnectionDetails(UnresolvedConnection &details)
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

void PeerTracker::OnResolvePorts(UnresolvedConnection details, service::Promise const &promise)
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

void PeerTracker::RegisterConnectionDetails(UnresolvedConnection const &details)
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

}  // namespace muddle
}  // namespace fetch