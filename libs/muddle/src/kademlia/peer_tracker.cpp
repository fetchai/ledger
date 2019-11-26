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

void PeerTracker::SetMuddlePorts(PortsList const &ports)
{
  peer_tracker_protocol_.SetMuddlePorts(ports);
  logging_name_ = "tcp://localhost:" + std::to_string(ports[0]);
}

PeerTracker::ConnectionPriorityMap PeerTracker::connection_priority() const
{
  FETCH_LOCK(mutex_);
  return longlived_connection_priority_;
}

void PeerTracker::SetConfiguration(TrackerConfiguration const &config)
{
  FETCH_LOCK(mutex_);
  tracker_configuration_ = config;
}

void PeerTracker::AddConnectionHandleToQueue(ConnectionHandle handle)
{
  FETCH_LOCK(mutex_);

  // Registering new handle if feature is enabled
  if (tracker_configuration_.register_connections)
  {
    UnresolvedConnection conn_details;
    conn_details.handle = handle;
    new_handles_.push(conn_details);
  }
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
  // Finding peers close to us
  auto peers = peer_table_.FindPeer(own_address_);

  // Adding the cloest known peers to the priority list
  for (auto &p : peers)
  {
    // Skipping own address
    if (p.address == own_address_)
    {
      continue;
    }

    // Creating a priority for the list
    AddressPriority priority;
    priority.address = p.address;
    priority.bucket  = GetBucket(p.distance);

    longlived_connection_priority_.insert({p.address, std::move(priority)});
  }

  //
  for (auto &p : longlived_connection_priority_)
  {
    // Skipping own address
    if (p.first == own_address_)
    {
      continue;
    }

    // Updating and pushing to list
    p.second.UpdatePriority();
    longlived_prioritized_peers_.push_back(p.second);
  }

  // Sorting according to priority
  std::sort(longlived_prioritized_peers_.begin(), longlived_prioritized_peers_.end(),
            [](auto const &a, auto const &b) -> bool {
              // Making highest priority appear first
              return a.priority > b.priority;
            });
}

void PeerTracker::UpdareLongLivedConnections()
{
  auto const currently_outgoing = register_.GetOutgoingAddressSet();
  auto const currently_incoming = register_.GetIncomingAddressSet();

  persistent_outgoing_connections_ = 0;
  keep_connections_.clear();
  no_uri_.clear();

  uint64_t n = 0;

  // Primary loop to ensure that we are conencted to high priority peers
  for (; n < longlived_prioritized_peers_.size(); ++n)
  {
    // If we have more longlived connections than the max,
    // we stop. The most important onces are now kept alive.
    if (persistent_outgoing_connections_ >= tracker_configuration_.max_longlived_outgoing)
    {

      break;
    }

    // Getting the peer details
    auto &p = longlived_prioritized_peers_[n];
    if (p.address == own_address_)
    {
      continue;
    }

    // Already connected treated peers are skipped
    if (keep_connections_.find(p.address) != keep_connections_.end())
    {
      continue;
    }

    // Ignoring addresses found in incoming
    if (currently_incoming.find(p.address) != currently_incoming.end())
    {
      continue;
    }

    // If not connected, we connect
    if (currently_outgoing.find(p.address) == currently_outgoing.end())
    {
      auto suri = peer_table_.GetUri(p.address);

      // If we are not connected, we connect.
      network::Uri uri;
      if (suri.empty())
      {
        no_uri_.insert(p.address);
        continue;
      }

      uri.Parse(suri);

      // TODO: record failure
      FETCH_LOG_WARN(logging_name_.c_str(), "Connecting to ", uri.ToString(), " with address ",
                     p.address.ToBase64());
      connections_.AddPersistentPeer(uri);
    }

    ++persistent_outgoing_connections_;

    // Keeping track of what we have connected to.
    keep_connections_.insert(p.address);
  }
}

void PeerTracker::DisconnectDuplicates()
{
  auto const outgoing = register_.GetOutgoingAddressSet();
  auto const incoming = register_.GetIncomingAddressSet();

  for (auto const &adr : outgoing)
  {
    if (incoming.find(adr) != incoming.end())
    {
      if (own_address_ < adr)
      {
        auto connections = register_.LookupConnections(adr);
        for (auto &conn_ptr : connections)
        {
          auto conn = conn_ptr.lock();
          if (conn && conn->Type() == network::AbstractConnection::TYPE_OUTGOING)
          {
            auto const handle = conn->handle();
            FETCH_LOG_WARN(logging_name_.c_str(), "Disconnecting from bilateral ", handle,
                           " connection: ", adr.ToBase64());

            // Note that the order of these two matters. RemovePersistentPeer must
            // be executed first.
            connections_.RemovePersistentPeer(handle);
            connections_.RemoveConnection(handle);
          }
        }
      }
    }
  }
}

void PeerTracker::DisconnectFromPeers()
{
  // Trimming connections
  auto connecting_to = register_.GetOutgoingAddressSet();
  if (connecting_to.size() <= tracker_configuration_.max_outgoing_connections)
  {
    return;
  }

  // Disconnecting from the remaining  nodes
  for (auto const &address : connecting_to)
  {
    if (keep_connections_.find(address) != keep_connections_.end())
    {
      continue;
    }

    auto connections = register_.LookupConnections(address);
    for (auto &conn_ptr : connections)
    {
      auto conn = conn_ptr.lock();
      if (conn && conn->Type() == network::AbstractConnection::TYPE_OUTGOING)
      {
        auto const handle = conn->handle();
        FETCH_LOG_WARN(logging_name_.c_str(), "Disconnecting from bilateral ", handle,
                       " connection: ", address.ToBase64());

        // Note that the order of these two matters. RemovePersistentPeer must
        // be executed first.
        connections_.RemovePersistentPeer(handle);
        connections_.RemoveConnection(handle);
      }
    }
  }
}

void PeerTracker::PullPeerKnowledge()
{
  // Scheduling for data pull
  if (peer_pull_queue_.empty())
  {
    auto const currently_connected_peers = register_.GetCurrentAddressSet();
    for (auto const &peer : currently_connected_peers)
    {
      SchedulePull(peer);
    }
  }

  // If it is still empty
  if (peer_pull_queue_.empty())
  {
    return;
  }

  auto search_for = own_address_;

  auto address = peer_pull_queue_.front();
  peer_pull_queue_.pop_front();

  // Increasing the tracker id.
  auto pull_id = pull_next_id_++;

  // make the call to the remote service
  auto promise = rpc_client_.CallSpecificAddress(address, RPC_MUDDLE_KADEMLIA,
                                                 PeerTrackerProtocol::FIND_PEERS, search_for);

  // wrap the promise is a task
  auto call_task =
      std::make_shared<PromiseTask>(promise, tracker_configuration_.promise_timeout,
                                    [this, address, pull_id](service::Promise const &promise) {
                                      OnResolvedPull(pull_id, address, promise);
                                    });

  // add the task to the reactor
  reactor_.Attach(call_task);

  // add the task to the pending resolutions queue
  pull_promises_.emplace(pull_id, std::move(call_task));
}

void PeerTracker::SchedulePull(Address const &address)
{
  // Checking whether it is already scheduled
  if (peer_pull_set_.find(address) != peer_pull_set_.end())
  {
    return;
  }

  // Pulling from self is not allowed
  if (address == own_address_)
  {
    return;
  }

  // Scheduling
  peer_pull_queue_.push_back(address);
  peer_pull_set_.insert(address);
}

PeerTracker::AddressSet PeerTracker::all_peers() const
{
  FETCH_LOCK(mutex_);
  return register_.GetCurrentAddressSet();
}

PeerTracker::AddressSet PeerTracker::incoming() const
{
  FETCH_LOCK(mutex_);
  return register_.GetIncomingAddressSet();
}

PeerTracker::AddressSet PeerTracker::outgoing() const
{
  FETCH_LOCK(mutex_);
  return register_.GetOutgoingAddressSet();
}

PeerTracker::AddressSet PeerTracker::keep_connections() const
{
  FETCH_LOCK(mutex_);
  return keep_connections_;
}

std::size_t PeerTracker::known_peer_count() const
{
  FETCH_LOCK(mutex_);
  return peer_table_.size();
}

PeerTracker::AddressSet PeerTracker::no_uri() const
{
  FETCH_LOCK(mutex_);
  return no_uri_;
}

void PeerTracker::OnResolvedPull(uint64_t pull_id, Address peer, service::Promise const &promise)
{
  FETCH_LOCK(mutex_);
  if (promise->state() == service::PromiseState::SUCCESS)
  {
    // Reporting that peer is still responding
    peer_table_.ReportLiveliness(peer);

    // extract the set of addresses from which the prospective node is contactable
    auto peer_info_list = promise->As<std::deque<PeerInfo>>();

    // create the new entry and populate
    // TODO: Work out if this is what we want
    for (auto const &peer_info : peer_info_list)
    {
      // reporting the possible existence of another peer on the network.a
      assert(peer_info.address.size() > 0);
      //      peer_table_.ReportLiveliness(peer_info.address, peer_info);
      peer_table_.ReportExistence(peer_info);
    }

    // Rescheduling task if not found.
    if (peer_pull_set_.find(peer) != peer_pull_set_.end())
    {
      peer_pull_queue_.push_back(peer);
    }
  }
  else
  {
    FETCH_LOG_WARN("TODO:logging_name_", "Unable to resolve address for: ", peer.ToBase64(),
                   " code: ", int(promise->state()));

    // In case of failure, we stop following
    if (peer_pull_set_.find(peer) != peer_pull_set_.end())
    {
      peer_pull_set_.erase(peer);
    }

    // and erase it from the table
    peer_table_.ReportFailure(peer);
  }

  // remove the entry from the pending list
  pull_promises_.erase(pull_id);
}

void PeerTracker::Periodically()
{
  FETCH_LOCK(mutex_);

  // Getting state of our connectivity
  auto const currently_connected_peers = register_.GetCurrentAddressSet();
  auto const currently_incoming        = register_.GetIncomingAddressSet();
  auto const currently_outgoing        = register_.GetOutgoingAddressSet();
  auto       ref                       = currently_outgoing;
  for (auto &adr : currently_incoming)
  {
    ref.insert(adr);
  }
  assert(ref.size() == currently_connected_peers.size());

  if (tracker_configuration_.register_connections)
  {
    // Resolving all handles and adding their details to the peer table.
    ProcessConnectionHandles();
  }

  //
  if (tracker_configuration_.pull_peers)
  {
    // Scheduling tracking every now and then
    PullPeerKnowledge();
  }

  if (tracker_configuration_.connect_to_nearest)
  {
    // Updating the connectivity priority list with the nearest known neighbours
    UpdatePriorityList();

    UpdareLongLivedConnections();
  }

  DisconnectDuplicates();

  DisconnectFromPeers();
}
/// @}

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

    // Invoking short-lived connection callback
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

  // Scheduling for data pull
  SchedulePull(details.address);
}

}  // namespace muddle
}  // namespace fetch