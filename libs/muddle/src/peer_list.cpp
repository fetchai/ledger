//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "muddle_logging_name.hpp"
#include "peer_list.hpp"
#include "router.hpp"

#include "logging/logging.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

static constexpr std::size_t MAX_LOG2_BACKOFF = 11;  // 2048
static constexpr char const *BASE_NAME        = "MuddlePeers";

namespace fetch {
namespace muddle {

PeerConnectionList::PeerConnectionList(NetworkId const &network)
  : name_{GenerateLoggingName(BASE_NAME, network)}
{}

void PeerConnectionList::SetStatusCallback(StatusCallback callback)
{
  status_callback_ = std::move(callback);
}

bool PeerConnectionList::AddPersistentPeer(Uri const &peer)
{
  FETCH_LOCK(lock_);
  auto const result = persistent_peers_.emplace(peer);
  return result.second;
}

void PeerConnectionList::RemovePersistentPeer(Uri const &peer)
{
  FETCH_LOCK(lock_);
  persistent_peers_.erase(peer);
}

void PeerConnectionList::RemovePersistentPeer(Handle handle)
{
  FETCH_LOCK(lock_);
  for (auto &peer_connection : peer_connections_)
  {
    if (peer_connection.second->handle() == handle)
    {
      persistent_peers_.erase(peer_connection.first);
      break;
    }
  }
}

std::size_t PeerConnectionList::GetNumPeers() const
{
  FETCH_LOCK(lock_);
  return persistent_peers_.size();
}

void PeerConnectionList::AddConnection(Uri const &peer, ConnectionPtr const &conn)
{
  FETCH_LOCK(lock_);
  // update the metadata
  auto &metadata     = peer_metadata_[peer];
  metadata.connected = false;
  ++metadata.attempts;

  peer_connections_[peer] = conn;
}

PeerConnectionList::PeerMap PeerConnectionList::GetCurrentPeers() const
{
  FETCH_LOCK(lock_);
  return peer_connections_;
}

PeerConnectionList::PeerSet PeerConnectionList::GetPersistentPeers() const
{
  FETCH_LOCK(lock_);
  return persistent_peers_;
}

bool PeerConnectionList::GetMetadataForPeer(Uri const &peer, PeerMetadata &metadata) const
{
  bool success{false};

  FETCH_LOCK(lock_);
  auto it = peer_metadata_.find(peer);
  if (it != peer_metadata_.end())
  {
    metadata = it->second;
    success  = true;
  }

  return success;
}

PeerConnectionList::ConnectionState PeerConnectionList::GetStateForPeer(Uri const &peer) const
{
  FETCH_LOCK(lock_);
  auto metadataiter = peer_metadata_.find(peer);
  if (metadataiter == peer_metadata_.end())
  {
    return ConnectionState::UNKNOWN;
  }
  auto const &metadata = metadataiter->second;

  if (metadata.connected)
  {
    return ConnectionState::CONNECTED;
  }

  if (ReadyForRetry(metadata))
  {
    return ConnectionState::TRYING;
  }

  return ConnectionState(int(ConnectionState::BACKOFF) + metadata.consecutive_failures);
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
  if ((connection_handle != 0u) && status_callback_)
  {
    status_callback_(peer, connection_handle, ConnectionState::CONNECTED);
  }

  FETCH_LOG_INFO(logging_name_, "Connection to ", peer.uri(),
                 " established (conn: ", connection_handle, ")");
}

void PeerConnectionList::RemoveConnection(Uri const &peer)
{
  FETCH_LOCK(lock_);

  // remove the active connection
  peer_connections_.erase(peer);

  // update the metadata
  auto mt_it = peer_metadata_.find(peer);
  if (mt_it != peer_metadata_.end())
  {
    auto &metadata = mt_it->second;
    ++metadata.consecutive_failures;
    ++metadata.total_failures;
    metadata.connected              = false;
    metadata.last_failed_connection = Clock::now();
  }
}

void PeerConnectionList::RemoveConnection(Handle handle)
{
  FETCH_LOCK(lock_);

  for (auto it = peer_connections_.begin(); it != peer_connections_.end(); ++it)
  {
    if (it->second->handle() == handle)
    {
      FETCH_LOG_DEBUG(logging_name_, "Connection to ", it->first.uri(), " lost");
      auto metadata = peer_metadata_.find(it->first);
      if (metadata != peer_metadata_.end())
      {
        metadata->second.connected = false;
      }
      peer_connections_.erase(it);
      break;
    }
  }
}

void PeerConnectionList::Disconnect(Uri const &peer)
{
  FETCH_LOCK(lock_);

  if (peer_metadata_.erase(peer) != 0u)
  {
    peer_connections_.erase(peer);
  }

  FETCH_LOG_DEBUG(logging_name_, "Connection to ", peer.uri(), " shut down");
}

void PeerConnectionList::DisconnectAll()
{
  FETCH_LOCK(lock_);
  peer_connections_.clear();
  persistent_peers_.clear();
}

bool PeerConnectionList::ReadyForRetry(PeerMetadata const &metadata) const
{
  std::size_t const log2_backoff = std::min(metadata.consecutive_failures, MAX_LOG2_BACKOFF);
  Timepoint const   backoff_deadline =
      metadata.last_failed_connection + std::chrono::seconds{1u << log2_backoff};
  return (Clock::now() >= backoff_deadline);
}

PeerConnectionList::PeerList PeerConnectionList::GetPeersToConnectTo() const
{
  PeerList peers;

  FETCH_LOCK(lock_);

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
        // look up the connection metadata
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
