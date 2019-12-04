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

#include "kademlia/peer_tracker.hpp"
#include "core/time/to_seconds.hpp"

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
  FETCH_LOCK(mutex_);
  return desired_peers_;
}

void PeerTracker::AddDesiredPeer(Address const &address, PeerTracker::Duration const &expiry)
{
  FETCH_LOCK(mutex_);
  FETCH_LOG_INFO(logging_name_.c_str(), "Desired peer by address: ", address.ToBase64());
  connection_expiry_.emplace(address, Clock::now() + expiry);
  desired_peers_.insert(address);
}

void PeerTracker::AddDesiredPeer(Address const &address, network::Peer const &hint,
                                 PeerTracker::Duration const &expiry)
{
  FETCH_LOCK(mutex_);
  connection_expiry_.emplace(address, Clock::now() + expiry);

  if (!address.empty())
  {
    desired_peers_.insert(address);
  }

  PeerInfo info;
  info.address = address;
  info.uri.Parse(hint.ToUri());
  peer_table_.ReportExistence(info, own_address_);

  desired_uris_.insert(info.uri);
  desired_uri_expiry_.emplace(info.uri, Clock::now() + expiry);
  FETCH_LOG_INFO(logging_name_.c_str(), "Desired peer by address and uri: ", info.uri.ToString(),
                 " - ", address.ToBase64());
}

void PeerTracker::AddDesiredPeer(PeerTracker::Uri const &uri, PeerTracker::Duration const &expiry)
{
  FETCH_LOCK(mutex_);
  desired_uris_.insert(uri);
  desired_uri_expiry_.emplace(uri, Clock::now() + expiry);
  FETCH_LOG_INFO(logging_name_.c_str(), "Desired peer by uri: ", uri.ToString());
}

void PeerTracker::RemoveDesiredPeer(Address const &address)
{
  FETCH_LOCK(mutex_);
  desired_peers_.erase(address);
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
        FETCH_LOG_INFO(logging_name_.c_str(), "Skipping URI request as already in progress.");
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

      FETCH_LOG_INFO(logging_name_.c_str(), "Connecting to ", uri.ToString(), " with address ",
                     p.address.ToBase64());
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
                      " connection: ", address.ToBase64());

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

  auto address    = peer_pull_queue_.front();
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
  FETCH_LOCK(mutex_);
  return desired_peers_;
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
    auto peer_info_list        = promise->As<std::deque<PeerInfo>>();
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

  for (auto &peer : desired_peers_)
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
    FETCH_LOG_INFO(logging_name_.c_str(), "Looking for connections to ", peer.ToBase64());

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

    // Ignoring addresses found in incoming
    if (currently_incoming.find(best_peer) != currently_incoming.end())
    {
      FETCH_LOG_ERROR(logging_name_.c_str(), "Connection to peer not found.");
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
        FETCH_LOG_WARN(logging_name_.c_str(), " - URI failed! ", peer.ToBase64());
        continue;
      }

      FETCH_LOG_INFO(logging_name_.c_str(), "Connecting to ", uri.ToString(), " with address ",
                     best_peer.ToBase64());
      connections_.AddPersistentPeer(uri);
    }

    // Keeping track of what we have connected to.
    if (best_peer != own_address_)
    {
      keep_connections_.insert(best_peer);
    }

    // Adding the peer to the track list.
    if (peer != best_peer)
    {
      SchedulePull(best_peer, peer);
    }
  }
}

void PeerTracker::Periodically()
{
  FETCH_LOCK(mutex_);

  // Clearing arrays used to track actions on connections
  keep_connections_.clear();
  no_uri_.clear();

  // Trimming for expired connections
  auto                                   now = Clock::now();
  std::unordered_map<Address, Timepoint> new_expiry;
  for (auto const &item : connection_expiry_)
  {

    // Keeping those which are still not expired
    if (item.second > now)
    {
      new_expiry.emplace(item);
    }
    else if (item.second < now)
    {
      // Deleting peers which has expired
      auto it = desired_peers_.find(item.first);
      if (it != desired_peers_.end())
      {
        desired_peers_.erase(it);
      }
    }
  }
  std::swap(connection_expiry_, new_expiry);

  // Trimming URIs
  std::unordered_map<Uri, Timepoint> new_uri_expiry;
  for (auto const &item : desired_uri_expiry_)
  {
    // Keeping those which are still not expired
    if (item.second > now)
    {
      new_uri_expiry.emplace(item);
    }
    else if (item.second < now)
    {
      // Deleting peers which has expired
      auto it = desired_uris_.find(item.first);
      if (it != desired_uris_.end())
      {
        desired_uris_.erase(it);
      }
    }
  }
  std::swap(desired_uri_expiry_, new_uri_expiry);

  // Ensuring that we keep connections open which we are currently
  // pulling data from
  for (auto const &item : uri_resolution_tasks_)
  {
    if (item.first != own_address_)
    {
      keep_connections_.emplace(item.first);
    }
  }

  // TODO(tfr): Add something similar for pulling

  // Converting URIs into addresses if possible
  std::unordered_set<Uri> new_uris;
  for (auto const &uri : desired_uris_)
  {
    if (peer_table_.HasUri(uri))
    {
      auto address = peer_table_.GetAddressFromUri(uri);
      FETCH_LOG_INFO(logging_name_.c_str(), "Address from URI found ", uri.ToString(), ": ",
                     address.ToBase64());

      // Moving expiry time accross based on address
      auto expit = desired_uri_expiry_.find(uri);
      if (expit != desired_uri_expiry_.end())
      {
        connection_expiry_[address] = expit->second;
        desired_uri_expiry_.erase(expit);
      }

      // Switching to address based desired peer
      if (address != own_address_)
      {
        desired_peers_.insert(std::move(address));
      }
    }
    else
    {
      new_uris.insert(uri);
    }
  }
  std::swap(new_uris, desired_uris_);

  // Adding the unresolved URIs to the connection pool
  for (auto const &uri : desired_uris_)
  {
    FETCH_LOG_INFO(logging_name_.c_str(), "Adding peer with unknown address: ", uri.ToString());
    connections_.AddPersistentPeer(uri);
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
    auto peers = peer_table_.FindPeer(own_address_);

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
      priority.bucket  = Bucket::IdByLogarithm(p.distance);

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
    auto uris    = promise->As<NetworkUris>();
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

}  // namespace muddle
}  // namespace fetch
