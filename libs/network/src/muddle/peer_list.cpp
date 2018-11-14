//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "network/muddle/peer_list.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/service_ids.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/router.hpp"

static constexpr std::size_t MAX_LOG2_BACKOFF = 11;  // 2048

namespace fetch {
namespace muddle {

/**
 * Create the peer connection list
 *
 * @param router The reference to the router
 */
  PeerConnectionList::PeerConnectionList(Router &router)
  : router_(router)
{}

void PeerConnectionList::AddPersistentPeer(Uri const &peer)
{
  FETCH_LOCK(lock_);
  persistent_peers_.emplace(peer);
}

void PeerConnectionList::RemovePersistentPeer(Uri const &peer)
{
  FETCH_LOCK(lock_);
  persistent_peers_.erase(peer);
}

std::size_t PeerConnectionList::GetNumPeers() const
{
  FETCH_LOCK(lock_);
  return persistent_peers_.size();
}

void PeerConnectionList::AddConnection(Uri const &peer, ConnectionPtr const &conn)
{
  Lock lock(lock_);
  peer_connections_[peer] = conn;

  // update the metadata
  auto &metadata     = peer_metadata_[peer];
  metadata.connected = false;
  ++metadata.attempts;
}

PeerConnectionList::PeerMap PeerConnectionList::GetCurrentPeers() const
{
  FETCH_LOCK(lock_);
  return peer_connections_;
}

PeerConnectionList::UriMap PeerConnectionList::GetUriMap() const
{
  UriMap map;

  // build the reverse map
  {
    FETCH_LOCK(lock_);
    for (auto const &element : peer_connections_)
    {
      map[element.second->handle()] = element.first;
    }
  }

  return map;
}

PeerConnectionList::ConnectionState PeerConnectionList::GetStateForPeer(Uri const &peer)
{
  FETCH_LOCK(lock_);
  auto metadataiter = peer_metadata_.find(peer);
  if (metadataiter == peer_metadata_.end())
  {
    return ConnectionState::UNKNOWN;
  }
  auto &metadata = metadataiter->second;
  if (metadata.connected)
  {
    return ConnectionState::CONNECTED;
  }
  if (ReadyForRetry(metadata))
  {
    return ConnectionState::TRYING;
  }
  else
  {
    return ConnectionState(int(ConnectionState::BACKOFF) + metadata.consecutive_failures);
  }
}

void PeerConnectionList::OnConnectionEstablished(Uri const &peer)
{
  Handle connection_handle = 0;

  // update the connection metadata
  {
    FETCH_LOCK(lock_);

    auto it = peer_connections_.find(peer);
    if (it != peer_connections_.end())
    {
      connection_handle = it->second->handle();
    }

    auto &metadata = peer_metadata_[peer];
    ++metadata.successes;
    metadata.connected            = true;
    metadata.consecutive_failures = 0;
  }

  // send an identity message
  if (connection_handle)
  {
    router_.AddConnection(connection_handle);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Connection to ", peer.uri(), " established");
}

void PeerConnectionList::RemoveConnection(Uri const &peer)
{
  FETCH_LOCK(lock_);

  // remove the active connection
  peer_connections_.erase(peer);

  // update the metadata
  auto &metadata = peer_metadata_[peer];
  ++metadata.consecutive_failures;
  ++metadata.total_failures;
  metadata.connected              = false;
  metadata.last_failed_connection = Clock::now();

  FETCH_LOG_INFO(LOGGING_NAME, "Connection to ", peer.uri(), " lost");
}

bool PeerConnectionList::ReadyForRetry(const PeerMetadata &metadata) const
{
  std::size_t const log2_backoff = std::min(metadata.consecutive_failures, MAX_LOG2_BACKOFF);
  Timepoint const   backoff_deadline =
      metadata.last_failed_connection + std::chrono::seconds{1 << log2_backoff};
  return (Clock::now() >= backoff_deadline);
}

PeerConnectionList::PeerList PeerConnectionList::GetPeersToConnectTo() const
{
  FETCH_LOCK(lock_);

  PeerList peers;

  // determine which of the persistent peers are no longer active
  for (auto const &peer : persistent_peers_)
  {
    bool const inactive = peer_connections_.find(peer) == peer_connections_.end();

    if (inactive)
    {
      auto it = peer_metadata_.find(peer);

      // determine if this is an initial connection, or if we should try and apply some
      // type of backoff.
      bool const is_first_connection = it == peer_metadata_.end();

      if (is_first_connection)
      {
        // on a first attempt, a connection attempt should always be made
        peers.push_back(peer);
      }
      else
      {
        // lookup the connection metadata
        auto const &metadata = it->second;

        // determine if this connection should be connected again
        if (ReadyForRetry(metadata))
        {
          peers.push_back(peer);
        }
      }
    }
  }

  return peers;
}

}  // namespace muddle
}  // namespace fetch
