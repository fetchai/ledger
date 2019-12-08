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

#include "core/time/to_seconds.hpp"
#include "kademlia/peer_tracker.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <queue>
#include <vector>

namespace fetch {
namespace muddle {
namespace {

std::string GenerateLoggingName(NetworkId const &network_id)
{
  static std::atomic<uint64_t> tc{0};
  return "PeerTracker:" + network_id.ToString() + "-" + std::to_string(++tc);
}

}  // namespace

PeerTracker::PeerTrackerPtr PeerTracker::New(PeerTracker::Duration const &interval,
                                             core::Reactor &reactor, MuddleRegister const &reg,
                                             PeerConnectionList &connections,
                                             MuddleEndpoint &    endpoint)
{
  return PeerTrackerPtr{new PeerTracker(interval, reactor, reg, connections, endpoint)};
}

PeerTracker::~PeerTracker()
{
  Stop();
}

void PeerTracker::Blacklist(Address const &target)
{
  blacklist_.Add(target);
}

void PeerTracker::Whitelist(Address const &target)
{
  blacklist_.Remove(target);
}

bool PeerTracker::IsBlacklisted(Address const &target) const
{
  return blacklist_.Contains(target);
}

PeerTracker::AddressSet PeerTracker::GetDesiredPeers() const
{
  return peer_table_.desired_peers();
}

void PeerTracker::AddDesiredPeer(Address const &address, PeerTracker::Duration const &expiry)
{
  assert(!address.empty());
  peer_table_.AddDesiredPeer(address, expiry);
  FETCH_LOG_DEBUG(logging_name_.c_str(), "Desired peer by address: ", address.ToBase64());
}

void PeerTracker::AddDesiredPeer(Address const &address, network::Peer const &hint,
                                 PeerTracker::Duration const &expiry)
{
  assert(!address.empty());
  peer_table_.AddDesiredPeer(address, hint, expiry);
  FETCH_LOG_DEBUG(logging_name_.c_str(), "Desired peer by address and uri: ", address.ToBase64());
}

void PeerTracker::AddDesiredPeer(PeerTracker::Uri const &uri, PeerTracker::Duration const &expiry)
{
  peer_table_.AddDesiredPeer(uri, expiry);
  FETCH_LOG_DEBUG(logging_name_.c_str(), "Desired peer by uri: ", uri.ToString());
}

void PeerTracker::RemoveDesiredPeer(Address const &address)
{
  peer_table_.RemoveDesiredPeer(address);
}

void PeerTracker::ReportSuccessfulConnectAttempt(Uri const &uri)
{
  peer_table_.ReportSuccessfulConnectAttempt(uri);
}

void PeerTracker::ReportFailedConnectAttempt(Uri const &uri)
{
  peer_table_.ReportFailedConnectAttempt(uri);
}

void PeerTracker::ReportLeaving(Uri const &uri)
{
  peer_table_.ReportLeaving(uri);
}

void PeerTracker::UpdateExternalUris(NetworkUris const &uris)
{
  peer_tracker_protocol_.UpdateExternalUris(uris);
}

void PeerTracker::UpdateExternalPorts(Ports const &ports)
{
  peer_tracker_protocol_.UpdateExternalPorts(ports);
}

PeerTracker::ConnectionPriorityMap PeerTracker::connection_priority() const
{
  FETCH_LOCK(mutex_);
  return kademlia_connection_priority_;
}

void PeerTracker::SetConfiguration(TrackerConfiguration const &config)
{
  FETCH_LOCK(mutex_);
  tracker_configuration_ = config;
}

void PeerTracker::AddConnectionHandle(ConnectionHandle handle)
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

void PeerTracker::RemoveConnectionHandle(ConnectionHandle handle)
{
  FETCH_LOCK(direct_mutex_);
  auto address = register_.GetAddress(handle);
  directly_connected_peers_.erase(address);
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
      // Checking if we are already pulling
      if (uri_resolution_tasks_.find(details.address) != uri_resolution_tasks_.end())
      {
        FETCH_LOG_DEBUG(logging_name_.c_str(), "Skipping URI request as already in progress.");
        continue;
      }

      // If not ports were detected, we request the client for its server ports
      if (details.uris.empty())
      {
        // make the call to the remote service
        auto promise = rpc_client_.CallSpecificAddress(details.address, RPC_MUDDLE_KADEMLIA,
                                                       PeerTrackerProtocol::GET_MUDDLE_URIS);

        // wrap the promise is a task
        auto task = std::make_shared<PromiseTask>(
            promise, tracker_configuration_.promise_timeout,
            [this, details](service::Promise const &promise) { OnResolveUris(details, promise); });

        // add the task to the reactor
        reactor_.Attach(task);

        // add the task to the pending resolutions queue
        uri_resolution_tasks_.emplace(details.address, std::move(task));
      }
    }
  }
  std::swap(new_handles_, not_resolved);
}

void PeerTracker::UpdatePriorityList(ConnectionPriorityMap & connection_priority,
                                     ConnectionPriorityList &prioritized_peers, Peers const &peers)
{
  // Adding the cloest known peers to the priority list
  for (auto const &p : peers)
  {
    // Skipping own address
    if (p.address == own_address_)
    {
      continue;
    }

    // Creating a priority for the list
    AddressPriority priority;
    priority.address = p.address;
    priority.bucket  = Bucket::IdByLogarithm(p.distance);

    connection_priority.insert({p.address, std::move(priority)});
  }

  //
  for (auto &p : connection_priority)
  {
    // Skipping own address
    if (p.first == own_address_)
    {
      continue;
    }

    // Updating and pushing to list
    p.second.UpdatePriority();
    prioritized_peers.push_back(p.second);
  }

  // Sorting according to priority
  std::sort(prioritized_peers.begin(), prioritized_peers.end(),
            [](auto const &a, auto const &b) -> bool {
              // Making highest priority appear first
              return a.priority > b.priority;
            });
}

void PeerTracker::ConnectToPeers(AddressSet &                  connections_made,
                                 ConnectionPriorityList const &prioritized_peers,
                                 uint64_t const &              max_connections)
{
  auto const currently_outgoing = register_.GetOutgoingAddressSet();
  auto const currently_incoming = register_.GetIncomingAddressSet();

  connections_made.clear();

  uint64_t n = 0;

  // Primary loop to ensure that we are conencted to high priority peers
  for (; n < prioritized_peers.size(); ++n)
  {
    // If we have more longlived connections than the max,
    // we stop. The most important onces are now kept alive.
    if (connections_made.size() >= max_connections)
    {
      break;
    }

    // Getting the peer details
    auto &p = prioritized_peers[n];
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
      auto uri = peer_table_.GetUri(p.address);

      // If we are not connected, we connect.
      if (!uri.IsValid())
      {
        no_uri_.insert(p.address);
        continue;
      }

      FETCH_LOG_DEBUG(logging_name_.c_str(), "Connecting to prioritised peer ", uri.ToString(),
                      " with address ", p.address.ToBase64());
      connections_.AddPersistentPeer(uri);
    }

    // Keeping track of what we have connected to.
    keep_connections_.insert(p.address);
    connections_made.insert(p.address);
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
            FETCH_LOG_DEBUG(logging_name_.c_str(), "Disconnecting from bilateral ", handle,
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

void PeerTracker::DisconnectFromSelf()
{
  // Disconnecting from self
  auto connections = register_.LookupConnections(own_address_);
  for (auto &conn_ptr : connections)
  {
    auto conn = conn_ptr.lock();
    if (conn && conn->Type() == network::AbstractConnection::TYPE_OUTGOING)
    {
      auto const handle = conn->handle();
      FETCH_LOG_DEBUG(logging_name_.c_str(), "Disconnecting from low priority peer ", handle,
                      " connection");

      // Note that the order of these two matters. RemovePersistentPeer must
      // be executed first.
      connections_.RemovePersistentPeer(handle);
      connections_.RemoveConnection(handle);
    }
  }
}

void PeerTracker::DisconnectFromPeers()
{
  // Trimming connections
  auto connecting_to = register_.GetOutgoingAddressSet();

  if (connecting_to.size() <= tracker_configuration_.max_kademlia_connections)
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
        FETCH_LOG_DEBUG(logging_name_.c_str(), "Disconnecting from low priority peer ", handle,
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

  // Searching in parallel to different nodes
  int64_t tasks_to_setup =
      tracker_configuration_.max_discovery_tasks - static_cast<int64_t>(pull_promises_.size());

  tasks_to_setup = std::min(tasks_to_setup, static_cast<int64_t>(peer_pull_queue_.size()));

  for (int64_t task = 0; task < tasks_to_setup; ++task)
  {
    auto address = peer_pull_queue_.front();
    if (address.size() != Packet::ADDRESS_SIZE)
    {
      continue;
    }

    auto search_for = peer_pull_map_[address];

    peer_pull_queue_.pop_front();

    // Increasing the tracker id.
    auto pull_id = pull_next_id_++;

    // make the call to the remote service
    auto promise = rpc_client_.CallSpecificAddress(address, RPC_MUDDLE_KADEMLIA,
                                                   PeerTrackerProtocol::FIND_PEERS, search_for);

    // wrap the promise is a task

    auto call_task = std::make_shared<PromiseTask>(
        promise, tracker_configuration_.promise_timeout,
        [this, address, search_for, pull_id](service::Promise const &promise) {
          OnResolvedPull(pull_id, address, search_for, promise);
        });

    // add the task to the reactor
    reactor_.Attach(call_task);

    // add the task to the pending resolutions queue
    pull_promises_.emplace(pull_id, std::move(call_task));
  }
}

void PeerTracker::SchedulePull(Address const &address)
{
  SchedulePull(address, own_address_);
}

void PeerTracker::SchedulePull(Address const &address, Address const &search_for)
{
  // Checking whether it is already scheduled
  if (peer_pull_map_.find(address) != peer_pull_map_.end())
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
  peer_pull_map_.emplace(address, search_for);
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

PeerTracker::AddressSet PeerTracker::longrange_connections() const
{
  FETCH_LOCK(mutex_);
  return longrange_connections_;
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

std::size_t PeerTracker::first_non_empty_bucket() const
{
  FETCH_LOCK(mutex_);
  return peer_table_.first_non_empty_bucket();
}

std::size_t PeerTracker::active_buckets() const
{
  FETCH_LOCK(mutex_);
  return peer_table_.active_buckets();
}

PeerTracker::AddressSet PeerTracker::no_uri() const
{
  FETCH_LOCK(mutex_);
  return no_uri_;
}

PeerTracker::AddressSet PeerTracker::desired_peers() const
{
  return peer_table_.desired_peers();
}

void PeerTracker::OnResolvedPull(uint64_t pull_id, Address const &peer, Address const &search_for,
                                 service::Promise const &promise)
{
  FETCH_LOCK(mutex_);
  if (promise->state() == service::PromiseState::SUCCESS)
  {
    // Reporting that peer is still responding
    peer_table_.ReportLiveliness(peer, own_address_);

    // extract the set of addresses from which the prospective node is contactable
    std::deque<PeerInfo> peer_info_list;
    promise->GetResult(peer_info_list);
    last_pull_from_peer_[peer] = Clock::now();

    // create the new entry and populate
    bool found = false;
    for (auto const &peer_info : peer_info_list)
    {
      // reporting the possible existence of another peer on the network.a
      assert(!peer_info.address.empty());

      peer_table_.ReportExistence(peer_info, peer);
      if (peer_info.address == search_for)
      {
        found = true;
      }
    }

    // Rescheduling task if not found.
    if (peer_pull_map_.find(peer) != peer_pull_map_.end())
    {
      if (found)
      {
        // We are done and remove the peer from the map
        peer_pull_map_.erase(peer);
      }
      else
      {
        // Otherwise we reshedule pull
        peer_pull_queue_.push_back(peer);
      }
    }
  }
  else
  {
    FETCH_LOG_DEBUG(logging_name_.c_str(), "Unable to resolve address for: ", peer.ToBase64(),
                    " code: ", int(promise->state()));

    // In case of failure, we stop following
    if (peer_pull_map_.find(peer) != peer_pull_map_.end())
    {
      peer_pull_map_.erase(peer);
    }

    // and erase it from the table
    peer_table_.ReportFailure(peer, own_address_);
  }

  // remove the entry from the pending list
  pull_promises_.erase(pull_id);
}

void PeerTracker::ConnectToDesiredPeers()
{
  auto const currently_outgoing = register_.GetOutgoingAddressSet();
  auto const currently_incoming = register_.GetIncomingAddressSet();

  for (auto &peer : desired_peers())
  {
    if (peer == own_address_)
    {
      continue;
    }

    // Ignoring addresses found in incoming
    if (currently_incoming.find(peer) != currently_incoming.end())
    {
      continue;
    }

    // Ignoring addresses found in incoming
    if (currently_outgoing.find(peer) != currently_outgoing.end())
    {
      keep_connections_.insert(peer);
      continue;
    }

    // Finding known details
    auto    known_details = peer_table_.GetPeerDetails(peer);
    Address best_peer;
    FETCH_LOG_DEBUG(logging_name_.c_str(), "Looking for connections to ", peer.ToBase64());

    // If we have details, we can connect directly
    if (known_details != nullptr)
    {
      best_peer = peer;
    }
    else
    {
      // Finding the peers closest to the desirec peer
      auto closest_peers = peer_table_.FindPeer(peer);

      // Skipping this peer if we cannot find any close peers
      if (closest_peers.empty())
      {
        continue;
      }

      // Finding the best peer (which might be the peer itself)
      uint64_t idx{0};
      do
      {
        best_peer = closest_peers[idx].address;

        // Skipping own address
        if (best_peer == own_address_)
        {
          ++idx;
          continue;
        }

        auto it = last_pull_from_peer_.find(best_peer);

        // Breaking if we never did a pull before
        if (it == last_pull_from_peer_.end())
        {
          break;
        }

        // Breaking if it is sufficiently long ago
        if (ToSeconds(Clock::now() - it->second) > 300)  // TODO(tfr): Fix parameter
        {
          break;
        }

        ++idx;
      } while (idx < closest_peers.size());

      //
      if (idx >= closest_peers.size())
      {
        continue;
      }
    }

    // Skipping own address
    if (best_peer == own_address_)
    {
      continue;
    }

    // If we are already connected, we schedule a pull
    if (currently_incoming.find(best_peer) != currently_incoming.end())
    {
      SchedulePull(best_peer, peer);
      continue;
    }

    // Ignoring addresses found in incoming
    if (currently_outgoing.find(best_peer) == currently_outgoing.end())
    {
      auto uri = peer_table_.GetUri(best_peer);

      // If we are not connected, we connect.
      if (!uri.IsValid())
      {
        no_uri_.insert(best_peer);
        FETCH_LOG_WARN(logging_name_.c_str(), "Uri not found for peer ", peer.ToBase64());
        continue;
      }

      FETCH_LOG_DEBUG(logging_name_.c_str(), "Connecting to desired peer ", uri.ToString(),
                      " with address ", best_peer.ToBase64());
      connections_.AddPersistentPeer(uri);
    }

    // Keeping track of what we have connected to.
    keep_connections_.insert(best_peer);

    // Adding the peer to the track list.
    if (peer != best_peer)
    {
      SchedulePull(best_peer, peer);
    }
  }
}

void PeerTracker::Periodically()
{
  if (stopping_)
  {
    return;
  }

  FETCH_LOCK(mutex_);

  // Clearing arrays used to track actions on connections
  keep_connections_.clear();
  no_uri_.clear();

  peer_table_.TrimDesiredPeers();

  // TODO(tfr): Add something similar for pulling

  // Converting URIs into addresses if possible
  {
    peer_table_.ConvertDesiredUrisToAddresses();

    // Adding the unresolved URIs to the connection pool
    for (auto const &uri : peer_table_.desired_uris())
    {
      FETCH_LOG_DEBUG(logging_name_.c_str(), "Adding peer with unknown address: ", uri.ToString());
      connections_.AddPersistentPeer(uri);
    }
  }

  if (tracker_configuration_.allow_desired_connections)
  {
    // Making connections to user defined endpoints
    ConnectToDesiredPeers();
  }

  if (tracker_configuration_.register_connections)
  {
    // Resolving all handles and adding their details to the peer table.
    ProcessConnectionHandles();
  }

  // Ensuring that we keep connections open which we are currently
  // pulling data from
  for (auto const &item : uri_resolution_tasks_)
  {
    if (item.first != own_address_)
    {
      keep_connections_.emplace(item.first);
    }
  }

  if (tracker_configuration_.pull_peers)
  {
    // Scheduling tracking every now and then
    PullPeerKnowledge();
  }

  if (tracker_configuration_.connect_to_nearest)
  {
    // Finding peers close to us
    auto peers = peer_table_.FindPeer(own_address_);

    // Updating the connectivity priority list with the nearest known neighbours
    UpdatePriorityList(kademlia_connection_priority_, kademlia_prioritized_peers_, peers);
    ConnectToPeers(kademlia_connections_, kademlia_prioritized_peers_,
                   tracker_configuration_.max_kademlia_connections);
  }

  if (tracker_configuration_.long_range_connectivity)
  {
    // TODO(tfr): Move into a functino
    // Finding peers close to us
    auto peers = peer_table_.FindPeerByHamming(own_address_);

    // Updating the connectivity priority list with the nearest known neighbours
    // Adding the cloest known peers to the priority list
    for (auto const &p : peers)
    {
      // Skipping own address
      if (p.address == own_address_)
      {
        continue;
      }

      // Creating a priority for the list
      AddressPriority priority;
      priority.address = p.address;
      priority.bucket  = Bucket::IdByHamming(p.distance);

      longrange_connection_priority_.insert({p.address, std::move(priority)});
    }

    //
    for (auto &p : longrange_connection_priority_)
    {
      // Skipping own address
      if (p.first == own_address_)
      {
        continue;
      }

      // Updating and pushing to list
      p.second.UpdatePriority();
      longrange_prioritized_peers_.push_back(p.second);
    }

    std::sort(longrange_prioritized_peers_.begin(), longrange_prioritized_peers_.end(),
              [](auto const &a, auto const &b) -> bool {
                // Making highest priority appear first
                return a.priority > b.priority;
              });

    ConnectToPeers(longrange_connections_, longrange_prioritized_peers_,
                   tracker_configuration_.max_longrange_connections);
  }

  // Identifying duplicate connections and remove them from the list
  if (tracker_configuration_.disconnect_duplicates)
  {
    DisconnectDuplicates();
  }

  // Enforces a maximum number of outgoing connections
  if (tracker_configuration_.trim_peer_list)
  {
    DisconnectFromPeers();
  }

  // Disconnecting from self if connected
  if (tracker_configuration_.disconnect_from_self)
  {
    DisconnectFromSelf();
  }

  // Finally, we create a list of accessible peers
  FETCH_LOCK(direct_mutex_);
  directly_connected_peers_.clear();
  auto const currently_outgoing = register_.GetOutgoingAddressSet();
  for (auto &p : currently_outgoing)
  {
    auto connection = register_.LookupConnection(p).lock();
    if (connection)
    {
      directly_connected_peers_.insert(p);
    }
  }

  // Adding incoming to the set
  auto const currently_incoming = register_.GetIncomingAddressSet();
  for (auto const &p : currently_incoming)
  {
    auto connection = register_.LookupConnection(p).lock();
    if (connection)
    {
      directly_connected_peers_.insert(p);
    }
  }

  // Dumping the tracker table
  peer_table_.Dump();  // TODO(tfr): make configurable.
}

PeerTracker::PeerTracker(PeerTracker::Duration const &interval, core::Reactor &reactor,
                         MuddleRegister const &reg, PeerConnectionList &connections,
                         MuddleEndpoint &endpoint)
  : core::PeriodicRunnable(interval)
  , logging_name_{GenerateLoggingName(endpoint.network_id())}
  , reactor_{reactor}
  , register_{reg}
  , endpoint_{endpoint}
  , connections_{connections}
  , peer_table_{endpoint_.GetAddress(), endpoint_.network_id()}
  , own_address_{endpoint_.GetAddress()}
  , rpc_client_{"PeerTracker", endpoint_, SERVICE_MUDDLE_PEER_TRACKER, CHANNEL_RPC}
  , rpc_server_(endpoint_, SERVICE_MUDDLE_PEER_TRACKER, CHANNEL_RPC)
  , peer_tracker_protocol_{peer_table_}

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

    // We can only rely on uris for outgoing connections.
    if (connection->Type() == network::AbstractConnection::TYPE_OUTGOING)
    {
      network::Peer peer{connection->Address(), connection->port()};

      Uri uri;
      uri.Parse(peer.ToUri());

      details.uris = NetworkUris({uri});
      RegisterConnectionDetails(details);
    }

    return ConnectionState::RESOLVED;
  }

  return ConnectionState::DEAD;
}

void PeerTracker::OnResolveUris(UnresolvedConnection details, service::Promise const &promise)
{
  FETCH_LOCK(mutex_);

  // Deleting task.
  uri_resolution_tasks_.erase(details.address);

  if (promise->state() == service::PromiseState::SUCCESS)
  {
    // extract the set of addresses from which the prospective node is contactable
    NetworkUris uris;
    promise->GetResult(uris);
    details.uris = std::move(uris);

    if (!details.uris.empty())
    {
      RegisterConnectionDetails(details);
    }
    else
    {
      FETCH_LOG_INFO(logging_name_.c_str(), "Peer returned an empty URI list.");
    }
  }
  else
  {
    FETCH_LOG_ERROR(logging_name_.c_str(),
                    "Failed retreiving Uris from peer: ", static_cast<uint64_t>(promise->state()));
  }
}

void PeerTracker::RegisterConnectionDetails(UnresolvedConnection const &details)
{
  // Reporting that peer is still responding
  if (!details.uris.empty())
  {
    PeerInfo info;

    info.address = details.address;
    info.uri     = details.uris[0];  // TODO(tfr): store all URIs

    peer_table_.ReportLiveliness(details.address, own_address_, info);

    // Scheduling for data pull
    SchedulePull(details.address);
  }
  else
  {
    FETCH_LOG_WARN(logging_name_.c_str(), "Could not resolve URI.");
  }
}

void PeerTracker::PrintRoutingReport(Address const &address) const
{
  std::stringstream ss("");

  // Finding best address
  Address          best_address{};
  KademliaDistance best = MaxKademliaDistance();
  ss << std::endl;
  ss << "Routing report" << std::endl;
  ss << "==============" << std::endl;
  // Comparing against own address
  auto target_kad = KademliaAddress::Create(address);
  auto own_kad    = KademliaAddress::Create(own_address_);
  auto own_dist   = GetKademliaDistance(target_kad, own_kad);
  ss << Bucket::IdByLogarithm(own_dist) << " " << Bucket::IdByHamming(own_dist) << ": "
     << own_address_.ToBase64() << std::endl;

  ss << "Peers: " << std::endl;
  {
    FETCH_LOCK(direct_mutex_);

    // Finding best address
    for (auto &peer : directly_connected_peers_)
    {
      KademliaAddress cmp  = KademliaAddress::Create(peer);
      auto            dist = GetKademliaDistance(target_kad, cmp);

      ss << Bucket::IdByLogarithm(dist) << " " << Bucket::IdByHamming(dist) << ": "
         << peer.ToBase64();
      if (dist < best)
      {
        ss << " *";
        best         = dist;
        best_address = peer;
      }
      if (dist < own_dist)
      {
        ss << " +";
      }
      ss << std::endl;
    }
  }
  std::cout << ss.str();
}

PeerTracker::Handle PeerTracker::LookupHandle(Address const &address)
{
  auto wptr = register_.LookupConnection(address);

  // If it is a direct connection we just return the handle
  auto connection = wptr.lock();
  if (connection)
  {
    // TODO: Causes deadlock peer_table_.ReportLiveliness(address, own_address_);
    return connection->handle();
  }
  // TODO(tfr): Create a cache for the search below

  // Finding best address
  Address own_copy = endpoint_.GetAddress();

  //    KademliaDistance                    best = MaxKademliaDistance();
  std::map<KademliaDistance, Address> candidates;

  {
    FETCH_LOCK(direct_mutex_);

    // Finding best address
    auto target_kad = KademliaAddress::Create(address);
    for (auto &peer : directly_connected_peers_)
    {
      KademliaAddress cmp  = KademliaAddress::Create(peer);
      auto            dist = GetKademliaDistance(target_kad, cmp);
      candidates[dist]     = peer;
    }

    // Comparing against own address
    auto own_kad     = KademliaAddress::Create(own_copy);
    auto dist        = GetKademliaDistance(target_kad, own_kad);
    candidates[dist] = own_address_;
  }

  for (auto &pair : candidates)
  {
    if (pair.second == own_copy)
    {
      return 0;
    }

    // Finding handle
    wptr       = register_.LookupConnection(pair.second);
    connection = wptr.lock();
    if (connection)
    {
      // TODO(tfr): add to cache for efficiency

      // TODO(tfr): Causes deadlock        peer_table_.ReportLiveliness(pair.second, own_address_);
      return connection->handle();
    }
  }
  return 0;
}

PeerTracker::Handle PeerTracker::LookupRandomHandle() const
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

void PeerTracker::Stop()
{
  FETCH_LOG_WARN(logging_name_.c_str(), "Stopping peer tracker.");
  FETCH_LOCK(mutex_);
  stopping_              = true;
  tracker_configuration_ = TrackerConfiguration::AllOff();

  peer_table_.ClearDesired();
  kademlia_connection_priority_.clear();
  kademlia_prioritized_peers_.clear();
  kademlia_connections_.clear();
  longrange_connection_priority_.clear();
  longrange_prioritized_peers_.clear();
  longrange_connections_.clear();
}

void PeerTracker::Start()
{
  stopping_ = false;
}

void PeerTracker::SetCacheFile(std::string const &filename)
{
  peer_table_.SetCacheFile(filename);
}

}  // namespace muddle
}  // namespace fetch
