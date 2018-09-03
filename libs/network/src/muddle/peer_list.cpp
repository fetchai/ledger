#include "core/service_ids.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "network/muddle/peer_list.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/router.hpp"

static constexpr std::size_t MAX_LOG2_BACKOFF = 11; // 2048

namespace fetch {
namespace muddle {

PeerConnectionList::PeerConnectionList(Router &router)
  : router_(router)
{
}

void PeerConnectionList::AddPersistentPeer(Peer const &peer)
{
  Lock lock(lock_);
  persistent_peers_.emplace(peer);
}

void PeerConnectionList::RemovePersistentPeer(Peer const &peer)
{
  Lock lock(lock_);
  persistent_peers_.erase(peer);
}

void PeerConnectionList::AddConnection(Peer const &peer, ConnectionPtr const &conn)
{
  Lock lock(lock_);
  peer_connections_[peer] = conn;

  // update the metadata
  auto &metadata = peer_metadata_[peer];
  ++metadata.attempts;
}

void PeerConnectionList::OnConnectionEstablished(PeerConnectionList::Peer const &peer)
{
  Handle connection_handle = 0;

  // update the connection metadata
  {
    Lock lock(lock_);

    auto it = peer_connections_.find(peer);
    if (it != peer_connections_.end())
    {
      connection_handle = it->second->handle();
    }

    auto &metadata = peer_metadata_[peer];
    ++metadata.successes;
    metadata.consecutive_failures = 0;
  }

  // send an identity message
  if (connection_handle)
  {
    router_.AddConnection(connection_handle);
  }
}

void PeerConnectionList::RemoveConnection(Peer const &peer)
{
  Lock lock(lock_);

#if 1
  auto it = peer_connections_.find(peer);
  if (it != peer_connections_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Connection ref count: ", it->second.use_count());
  }
#endif

  peer_connections_.erase(peer);

  // update the metadata
  auto &metadata = peer_metadata_[peer];
  ++metadata.consecutive_failures;
  ++metadata.total_failures;

  FETCH_LOG_INFO(LOGGING_NAME, "Failed time before: ", metadata.last_failed_connection.time_since_epoch().count());

  metadata.last_failed_connection = Clock::now();

  FETCH_LOG_INFO(LOGGING_NAME, "Failed time after: ", metadata.last_failed_connection.time_since_epoch().count());


  FETCH_LOG_INFO(LOGGING_NAME, "I do dismay, it appears we have lost a connection");
}

PeerConnectionList::PeerList PeerConnectionList::GetPeersToConnectTo() const
{
  Lock lock(lock_);

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
        // first attempt should always be checked
        peers.push_back(peer);
      }
#if 1
      else
      {
        auto const &metadata = it->second;

        // since we have metadata, this is not the initial connection
        std::size_t const log2_backoff = std::min(metadata.consecutive_failures, MAX_LOG2_BACKOFF);
        Timepoint const backoff_deadline = metadata.last_failed_connection + std::chrono::seconds{1 << log2_backoff};

        FETCH_LOG_INFO(LOGGING_NAME, "Curiously determining if I need to back off ", 1 << log2_backoff);

        if (Clock::now() >= backoff_deadline)
        {
          peers.push_back(peer);

          FETCH_LOG_INFO(LOGGING_NAME, "Come forth dear fellow: ", peer.ToString());
        }
      }
#endif
    }
  }

  return peers;
}

} // namespace p2p
} // namespace fetch
