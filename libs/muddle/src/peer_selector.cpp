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

#include "core/containers/is_in.hpp"
#include "core/containers/set_difference.hpp"
#include "core/containers/set_intersection.hpp"
#include "core/containers/set_join.hpp"
#include "core/mutex.hpp"
#include "core/reactor.hpp"
#include "core/service_ids.hpp"
#include "direct_message_service.hpp"
#include "discovery_service.hpp"
#include "muddle_logging_name.hpp"
#include "muddle_register.hpp"
#include "peer_list.hpp"
#include "peer_selector.hpp"
#include "xor_metric.hpp"

#include <cstddef>
#include <unordered_set>

namespace fetch {
namespace muddle {

namespace {

using namespace std::chrono;
using namespace std::chrono_literals;

constexpr auto        MIN_ANNOUNCEMENT_INTERVAL = 10s;
constexpr auto        MAX_ANNOUNCEMENT_INTERVAL = 30s;
constexpr std::size_t MINIMUM_PEERS             = 6;
constexpr char const *BASE_NAME                 = "PeerSelector";
constexpr std::size_t MAX_CACHE_KAD_NODES       = 20;
constexpr std::size_t MAX_CONNECTED_KAD_NODES   = 8;
constexpr std::size_t MAX_LOG2_BACKOFF          = 11;  // 2048

PromiseTask::Duration CalculatePromiseTimeout(std::size_t consecutive_failures)
{
  std::size_t const log2_backoff_secs = std::min(consecutive_failures, MAX_LOG2_BACKOFF);
  return duration_cast<PromiseTask::Duration>(seconds{1 << log2_backoff_secs});
}

}  // namespace

PeerSelector::PeerSelector(NetworkId const &network, Duration const &interval,
                           core::Reactor &reactor, MuddleRegister const &reg,
                           PeerConnectionList &connections, MuddleEndpoint &endpoint)
  : core::PeriodicRunnable(interval)
  , name_{GenerateLoggingName(BASE_NAME, network)}
  , reactor_{reactor}
  , connections_{connections}
  , register_{reg}
  , endpoint_{endpoint}
  , address_{endpoint_.GetAddress()}
  , rpc_client_{"PeerSelect", endpoint_, SERVICE_MUDDLE, CHANNEL_RPC}
  , announcement_subscription_{endpoint_.Subscribe(SERVICE_MUDDLE, CHANNEL_ANNOUNCEMENT)}
{
  announcement_subscription_->SetMessageHandler(this, &PeerSelector::OnAnnouncement);
}

void PeerSelector::AddDesiredPeer(Address const &address)
{
  FETCH_LOCK(lock_);
  desired_addresses_.emplace(address);
}

void PeerSelector::AddDesiredPeer(Address const &address, network::Peer const &hint)
{
  FETCH_LOCK(lock_);
  desired_addresses_.emplace(address);

  auto &info = peers_info_[address];

  // try and find the hint in the peer list
  bool const hint_not_present = std::find_if(info.peer_data.begin(), info.peer_data.end(),
                                             [&hint](PeerMetadata const &metadata) {
                                               return metadata.peer == hint;
                                             }) == info.peer_data.end();

  if (hint_not_present)
  {
    info.peer_data.emplace_back(PeerMetadata{hint});
  }
}

void PeerSelector::RemoveDesiredPeer(Address const &address)
{
  FETCH_LOCK(lock_);
  desired_addresses_.erase(address);
}

PeerSelector::Addresses PeerSelector::GetDesiredPeers() const
{
  FETCH_LOCK(lock_);
  return desired_addresses_;
}

PeerSelector::Addresses PeerSelector::GetKademliaPeers() const
{
  FETCH_LOCK(lock_);
  return kademlia_addresses_;
}

PeerSelector::Addresses PeerSelector::GetPendingRequests() const
{
  Addresses addresses;

  {
    FETCH_LOCK(lock_);
    for (auto const &element : pending_resolutions_)
    {
      addresses.emplace(element.first);
    }
  }

  return addresses;
}

PeerSelector::PeersInfo PeerSelector::GetPeerCache() const
{
  FETCH_LOCK(lock_);
  return peers_info_;
}

PeerSelectionMode PeerSelector::GetMode() const
{
  FETCH_LOCK(lock_);
  return mode_;
}

void PeerSelector::SetMode(PeerSelectionMode mode)
{
  FETCH_LOCK(lock_);

  auto const previous_mode{mode_};
  mode_ = mode;

  if (previous_mode != mode_)
  {
    switch (mode_)
    {
    case PeerSelectionMode::DEFAULT:
      kademlia_addresses_.clear();
      break;
    case PeerSelectionMode::KADEMLIA:
      announcement_interval_.Restart(0);
      break;
    }
  }
}

void PeerSelector::UpdatePeers(Peers peers)
{
  FETCH_LOCK(lock_);
  external_peers_ = std::move(peers);
}

void PeerSelector::Periodically()
{
  FETCH_LOCK(lock_);

  // kademlia selection
  if (PeerSelectionMode::KADEMLIA == mode_)
  {
    MakeAnnouncement();
    UpdateKademliaPeers();
  }

  // get the set of addresses to which we want to connect to
  auto const currently_connected_peers = register_.GetCurrentAddressSet();
  auto const current_outgoing_peers    = register_.GetOutgoingAddressSet();
  auto const target_peers              = desired_addresses_ + kademlia_addresses_;
  auto const outstanding_peers         = target_peers - currently_connected_peers;
  auto const unwanted_peers            = current_outgoing_peers - desired_addresses_;

  // resolve any outstanding unknown addresses
  ResolveAddresses(outstanding_peers);

  // generate the next set of uris
  auto const next_uris = GenerateUriSet(outstanding_peers);

  // determine all the additions that should be made first
  auto const additions = next_uris - connections_.GetPersistentPeers();

  // add all the additions
  for (auto const &uri : additions)
  {
    connections_.AddPersistentPeer(uri);
  }

  // we only start removing connections once we have applied all the additions
  if (additions.empty())
  {
    auto const num_remaining_peers = current_outgoing_peers.size() - unwanted_peers.size();

    // since we need to ensure connectivity is kept alive, we need will not start removing peers
    // until we have
    if (num_remaining_peers >= MINIMUM_PEERS)
    {
      for (auto const &address : unwanted_peers)
      {
        // look up the connection from its address
        auto conn = register_.LookupConnection(address).lock();
        if (conn)
        {
          FETCH_LOG_WARN(logging_name_, "Dropping Address: ", address.ToBase64());

          auto const handle = conn->handle();

          connections_.RemoveConnection(handle);
          connections_.RemovePersistentPeer(handle);
        }
      }
    }
  }
}

void PeerSelector::ResolveAddresses(Addresses const &addresses)
{
  // generate the set of addresses which have not been resolved yet
  auto unresolved_addresses = addresses - pending_resolutions_;

  // additionally filter the unresolved addresses by the peers which we don't have information for
  for (auto const &peer : peers_info_)
  {
    if (unresolved_addresses.find(peer.first) != unresolved_addresses.end())
    {
      if (!peer.second.peer_data.empty())
      {
        unresolved_addresses.erase(peer.first);
      }
    }
  }

  for (auto const &address : unresolved_addresses)
  {
    // make the call to the remote service
    auto promise = rpc_client_.CallSpecificAddress(address, RPC_MUDDLE_DISCOVERY,
                                                   DiscoveryService::CONNECTION_INFORMATION);
    // look up the peer information
    auto const &peer_data = peers_info_[address];

    // wrap the promise is a task
    auto task = std::make_shared<PromiseTask>(
        promise, CalculatePromiseTimeout(peer_data.consecutive_failures),
        [this, address](service::Promise const &promise) { OnResolvedAddress(address, promise); });

    // add the task to the reactor
    reactor_.Attach(task);

    // add the task to the pending resolutions queue
    pending_resolutions_.emplace(address, std::move(task));
  }
}

void PeerSelector::OnResolvedAddress(Address const &address, service::Promise const &promise)
{
  if (promise->state() == service::PromiseState::SUCCESS)
  {
    // extract the set of addresses from which the prospective node is contactable
    auto peer_addresses = promise->As<std::vector<network::Peer>>();

    // remove any previous entries for this address (avoid stale information)
    peers_info_.erase(address);

    FETCH_LOG_TRACE(logging_name_, "Successful resolution for ", address.ToBase64());

    // create the new entry and populate
    auto &metadata = peers_info_[address];
    for (auto const &peer_address : peer_addresses)
    {
      FETCH_LOG_TRACE(logging_name_, "- Candidate: ", peer_address.ToString());
      metadata.peer_data.emplace_back(peer_address);
    }
  }
  else
  {
    FETCH_LOG_WARN(logging_name_, "Unable to resolve address for: ", address.ToBase64(),
                   " code: ", int(promise->state()));

    // update the failure
    auto &peer_data = peers_info_[address];
    peer_data.consecutive_failures++;
  }

  // remove the entry from the pending list
  pending_resolutions_.erase(address);
}

PeerSelector::UriSet PeerSelector::GenerateUriSet(Addresses const &addresses)
{
  UriSet uris{};
  for (auto const &address : addresses)
  {
    auto it = peers_info_.find(address);
    if (it != peers_info_.end())
    {
      auto &metadata = it->second;

      if (metadata.peer_index < metadata.peer_data.size())
      {
        auto &current_peer = metadata.peer_data[metadata.peer_index];

        // ignore unreachable peers
        if (current_peer.unreachable)
        {
          continue;
        }

        network::Uri current_uri{current_peer.peer};

        // update the unreachable status if necessary
        PeerConnectionList::PeerMetadata connection_metadata{};
        if (connections_.GetMetadataForPeer(current_uri, connection_metadata))
        {
          // if we have not been able to establish a connection then we should give up after the
          // specified number of attempts
          if ((!connection_metadata.connected) && (connection_metadata.consecutive_failures >= 6))
          {
            FETCH_LOG_CRITICAL(logging_name_, "Marking ", current_uri.ToString(),
                               " as unreachable");
            current_peer.unreachable = true;
          }
        }

        if (!current_peer.unreachable)
        {
          FETCH_LOG_TRACE(logging_name_, "Mapped ", address.ToBase64(), " to ",
                          current_uri.ToString());

          uris.emplace(std::move(current_uri));
          break;  // ensure we do not populate multiple entries for a single address
        }
      }
    }
  }

  return uris;
}

void PeerSelector::OnAnnouncement(Address const &from, byte_array::ConstByteArray const &payload)
{
  static constexpr auto CACHE_LIFETIME = MAX_ANNOUNCEMENT_INTERVAL + MIN_ANNOUNCEMENT_INTERVAL;
  FETCH_UNUSED(payload);

  // load the peer list from the network
  Peers peer_list{};
  try
  {
    serializers::MsgPackSerializer serializer{payload};
    serializer >> peer_list;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(logging_name_, "Unable to deserialise announcement packet: ", ex.what());
  }

  // convert to a set
  std::unordered_set<network::Peer> const peers{peer_list.begin(), peer_list.end()};

  FETCH_LOG_TRACE(logging_name_, "Received announcement from: ", from.ToBase64());

  FETCH_LOCK(lock_);

  {
    // attempt to locate the existing node
    auto const it = std::find_if(kademlia_nodes_.begin(), kademlia_nodes_.end(),
                                 [&](KademliaNode const &node) { return node.address == from; });

    if (it != kademlia_nodes_.end())
    {
      // clear the lifetime
      it->lifetime.Restart(CACHE_LIFETIME);
    }
    else
    {
      // add the node into the list
      kademlia_nodes_.emplace_back(KademliaNode{from, CACHE_LIFETIME});

      // sort by distance
      kademlia_nodes_.sort([this](KademliaNode const &a, KademliaNode const &b) {
        auto const distance_a = CalculateDistance(address_, a.address);
        auto const distance_b = CalculateDistance(address_, b.address);

        if (distance_a == distance_b)
        {
          return a.address < b.address;
        }

        return distance_a < distance_b;
      });

      // trim the kademlia cache nodes
      while (kademlia_nodes_.size() > MAX_CACHE_KAD_NODES)
      {
        kademlia_nodes_.pop_back();
      }
    }
  }

  // remove all internal references to this objects address and peer
  std::unordered_set<Address> removed_addresses{};
  {
    for (auto it = peers_info_.begin(); it != peers_info_.end();)
    {
      Metadata &metadata  = it->second;
      auto &    peer_data = metadata.peer_data;

      // remove the peer address from the cache
      bool peers_removed{false};
      for (auto peer_it = peer_data.begin(); peer_it != peer_data.end();)
      {
        if (core::IsIn(peers, peer_it->peer))
        {
          peer_it       = peer_data.erase(peer_it);
          peers_removed = true;
        }
        else
        {
          ++peer_it;
        }
      }

      // remove the entry completely if we have removed all previous peers
      if (peers_removed && peer_data.empty())
      {
        removed_addresses.emplace(it->first);
        it = peers_info_.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  // for all the addresses that have been removed, they must be removed from the desired and kad
  // peers lists too.
  desired_addresses_  = desired_addresses_ - removed_addresses;
  kademlia_addresses_ = kademlia_addresses_ - removed_addresses;

  Metadata *metadata{nullptr};

  // lookup or create the metadata object
  auto it = peers_info_.find(from);
  if (it != peers_info_.end())
  {
    // lookup existing entry
    metadata = &(it->second);
  }
  else
  {
    // create a new entry
    metadata = &peers_info_[from];
  }

  if (metadata != nullptr)
  {
    PeerData &peer_data = metadata->peer_data;

    peer_data.reserve(peer_data.size() + peers.size());
    for (auto const &peer : peers)
    {
      peer_data.emplace_back(PeerMetadata{peer});
    }
  }
}

void PeerSelector::ScheduleNextAnnouncement()
{
  static constexpr uint64_t MIN_ANNOUNCE_TIME_MS =
      duration_cast<milliseconds>(MIN_ANNOUNCEMENT_INTERVAL).count();
  static constexpr uint64_t MAX_ANNOUNCE_TIME_MS =
      duration_cast<milliseconds>(MAX_ANNOUNCEMENT_INTERVAL).count();
  static constexpr uint64_t DELTA_ANNOUNCE_TIME_MS = MAX_ANNOUNCE_TIME_MS - MIN_ANNOUNCE_TIME_MS;
  static_assert(MIN_ANNOUNCE_TIME_MS < MAX_ANNOUNCE_TIME_MS, "Min must be smaller than max");
  static_assert(DELTA_ANNOUNCE_TIME_MS != 0, "Delta can't be zero");

  uint64_t const next_interval = (rng_() % DELTA_ANNOUNCE_TIME_MS) + MIN_ANNOUNCE_TIME_MS;
  announcement_interval_.Restart(next_interval);
}

void PeerSelector::MakeAnnouncement()
{
  if (announcement_interval_.HasExpired())
  {
    if (endpoint_.GetDirectlyConnectedPeerSet().empty())
    {
      announcement_interval_.Restart(1000);

      FETCH_LOG_TRACE(logging_name_, "Aborting kad announcement");
      return;
    }

    FETCH_LOG_TRACE(logging_name_, "Making kad announcement");

    serializers::LargeObjectSerializeHelper serialiser{};
    serialiser << external_peers_;

    // send out the announcement
    endpoint_.Broadcast(SERVICE_MUDDLE, CHANNEL_ANNOUNCEMENT, serialiser.data());

    // schedule the next announcement
    ScheduleNextAnnouncement();
  }
}

void PeerSelector::UpdateKademliaPeers()
{
  kademlia_addresses_.clear();

  for (auto it = kademlia_nodes_.begin(); it != kademlia_nodes_.end();)
  {
    if (it->lifetime.HasExpired())
    {
      // removed the expired nodes
      it = kademlia_nodes_.erase(it);
      continue;
    }

    // add the element into the set
    kademlia_addresses_.emplace(it->address);

    // exit if we have reached the end of the requested nodes
    if (kademlia_addresses_.size() >= MAX_CONNECTED_KAD_NODES)
    {
      break;
    }

    ++it;
  }
}

}  // namespace muddle
}  // namespace fetch
