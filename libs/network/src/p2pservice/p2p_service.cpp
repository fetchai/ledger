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

#include "network/p2pservice/p2p_service.hpp"
#include "core/containers/set_difference.hpp"
#include "network/p2pservice/manifest.hpp"

#include <algorithm>
#include <iterator>
#include <unordered_set>

namespace fetch {
namespace p2p {

P2PService::P2PService(Muddle &muddle, LaneManagement &lane_management, TrustInterface &trust,
                       std::size_t max_peers, std::size_t transient_peers,
                       uint32_t peer_update_cycle_ms)
  : muddle_(muddle)
  , muddle_ep_(muddle.AsEndpoint())
  , lane_management_{lane_management}
  , trust_system_{trust}
  , address_(muddle.identity().identifier())
  , identity_cache_{}
  , resolver_{identity_cache_}
  , resolver_proto_{resolver_, *this}
  , client_("R:P2P", muddle_ep_, Muddle::Address(), SERVICE_P2P, CHANNEL_RPC)
  , local_services_(lane_management_)
  , max_peers_(max_peers)
  , transient_peers_(transient_peers)
  , peer_update_cycle_ms_(peer_update_cycle_ms)
  , manifest_update_cycle_ms_(2000)
{
  // register the services with the rpc server
  rpc_server_.Add(RPC_P2P_RESOLVER, &resolver_proto_);
}

void P2PService::Start(UriList const &initial_peer_list)
{
  Uri const p2p_uri = manifest_.GetUri(ServiceIdentifier{ServiceType::CORE});

  resolver_.Setup(address_, p2p_uri);

  FETCH_LOG_INFO(LOGGING_NAME, "CORE URI: ", p2p_uri.uri());
  FETCH_LOG_INFO(LOGGING_NAME, "Num Initial Peers: ", initial_peer_list.size());
  for (auto const &uri : initial_peer_list)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Initial Peer: ", uri.uri());

    // trust this peer
    muddle_.AddPeer(uri);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Starting P2PService...");

  thread_pool_->SetIdleInterval(1000);
  thread_pool_->Start();
  thread_pool_->PostIdle([this]() { WorkCycle(); });

  process_future_timepoint_.Set(peer_update_cycle_ms_);
  manifests_next_update_timepoint_.Set(manifest_update_cycle_ms_);
}

void P2PService::Stop()
{
  thread_pool_->Clear();
  thread_pool_->Stop();
}

void P2PService::WorkCycle()
{
  if (peer_update_cycle_ms_.count() > 0 && process_future_timepoint_.IsDue())
  {
    // Important, set the cycle time early to prevent rapid re-calls
    // in the case of exceptions.
    process_future_timepoint_.Set(peer_update_cycle_ms_);

    AddressSet    active_addresses;
    ConnectionMap active_connections;

    // get the summary of all the current connections
    GetConnectionStatus(active_connections, active_addresses);

    // update our identity cache (address -> uri mapping)
    identity_cache_.Update(active_connections);

    // update the trust system with current connection information
    UpdateTrustStatus(active_connections);

    // discover new good peers on the network
    PeerDiscovery(active_addresses);

    // make the decisions about which peers are desired and which ones we now need to drop
    RenewDesiredPeers(active_addresses);

    // perform connections updates and drops based on previous step
    UpdateMuddlePeers(active_addresses);
  }

  if (manifest_update_cycle_ms_.count() > 0 && manifests_next_update_timepoint_.IsDue())
  {
    // Important, set the cycle time early to prevent rapid re-calls
    // in the case of exceptions.
    process_future_timepoint_.Set(manifest_update_cycle_ms_);

    AddressSet active_addresses;

    // get a list of the direct connections for this node
    auto const connections = muddle_.GetConnections(true);

    for (auto const &connection : connections)
    {
      if (Uri::Scheme::Tcp == connection.second.scheme())
      {
        active_addresses.insert(connection.first);
      }
    }

    UpdateManifests(active_addresses);
  }
}

void P2PService::GetConnectionStatus(ConnectionMap &active_connections,
                                     AddressSet &   active_addresses)
{
  // get a summary of addresses and associated URIs
  active_connections = muddle_.GetConnections();

  // generate the set of addresses to whom we are currently connected
  active_addresses.reserve(active_connections.size());

  for (const auto &c : active_connections)
  {
    if (muddle_.IsConnected(c.first))
    {
      active_addresses.insert(c.first);
    }
  }
}

void P2PService::UpdateTrustStatus(ConnectionMap const &active_connections)
{
  for (auto const &element : active_connections)
  {
    auto const &address = element.first;

    // ensure that the trust system is informed of new addresses
    if (!trust_system_.IsPeerKnown(address))
    {
      trust_system_.AddFeedback(address, TrustSubject::PEER, TrustQuality::NEW_PEER);
    }
  }

  for (auto const &pt : trust_system_.GetPeersAndTrusts())
  {
    auto        address = pt.address;
    std::string name(ToBase64(address));

    // update our desired
    bool const new_peer     = desired_peers_.find(address) == desired_peers_.end();
    bool const trusted_peer = trust_system_.IsPeerTrusted(address);

    if (new_peer && trusted_peer)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Trusting: ", ToBase64(address));
      desired_peers_.insert(address);
    }

    if (!trusted_peer)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Untrusting ", ToBase64(address),
                     " because trust=", trust_system_.GetTrustRatingOfPeer(address));
      desired_peers_.erase(address);
      if (trust_system_.GetTrustRatingOfPeer(address) < 0.0)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Blacklisting ", ToBase64(address),
                       " because trust=", trust_system_.GetTrustRatingOfPeer(address));
        blacklisted_peers_.insert(address);
      }
    }
  }
}

void P2PService::PeerDiscovery(AddressSet const &active_addresses)
{
  for (auto const &address : pending_peer_lists_.FilterOutInFlight(active_addresses))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Discover new peers from: ", ToBase64(address));

    auto prom = network::PromiseOf<AddressSet>(client_.CallSpecificAddress(
        address, RPC_P2P_RESOLVER, ResolverProtocol::GET_RANDOM_GOOD_PEERS));
    pending_peer_lists_.Add(address, prom);
  }

  // resolve the any remaining promises
  pending_peer_lists_.Resolve();

  // process any peer discovery updates that are returned from the queue
  for (auto &result : pending_peer_lists_.Get(MAX_PEERS_PER_CYCLE))
  {
    Address const &from      = result.key;
    AddressSet &   addresses = result.promised;

    // ensure that our own address is removed if included in the address set
    addresses.erase(address_);

    if (!addresses.empty())
    {
      for (auto const &new_address : addresses)
      {
        // update the trust
        if (!trust_system_.IsPeerKnown(new_address))
        {
          FETCH_LOG_INFO(LOGGING_NAME, "Discovered peer: ", ToBase64(new_address),
                         " (from: ", ToBase64(from), ")");

          trust_system_.AddFeedback(new_address, TrustSubject::PEER, TrustQuality::NEW_PEER);

          trust_system_.AddFeedback(from, TrustSubject::PEER, TrustQuality::NEW_INFORMATION);
        }
      }
    }
  }
}

bool P2PService::IsDesired(Address const &address)
{
  return desired_peers_.find(address) != desired_peers_.end();
}

void P2PService::RenewDesiredPeers(AddressSet const & /*active_addresses*/)
{
  auto static_peers       = trust_system_.GetBestPeers(max_peers_ - transient_peers_);
  auto experimental_peers = trust_system_.GetRandomPeers(transient_peers_, 0.0);

  desired_peers_.clear();

  for (auto const &p : static_peers)
  {
    desired_peers_.insert(p);
  }

  for (auto const &p : experimental_peers)
  {
    desired_peers_.insert(p);
  }
}

void P2PService::UpdateMuddlePeers(AddressSet const &active_addresses)
{
  static constexpr std::size_t MAX_RESOLUTIONS_PER_CYCLE = 20;

  AddressSet const outgoing_peers = identity_cache_.FilterOutUnresolved(active_addresses);

  AddressSet const new_peers     = desired_peers_ - active_addresses;
  AddressSet const dropped_peers = outgoing_peers - desired_peers_;

  for (auto const &d : desired_peers_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Muddle Update: KEEP: ", ToBase64(d));
    Uri uri;
    if (identity_cache_.Lookup(d, uri) && uri.IsDirectlyConnectable())
    {
      muddle_.AddPeer(uri);
    }
  }
  for (auto const &d : dropped_peers)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Muddle Update: LOSE: ", ToBase64(d));
  }
  for (auto const &d : new_peers)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Muddle Update: GAIN: ", ToBase64(d));
  }

  // process pending resolutions
  pending_resolutions_.Resolve();
  for (auto const &result : pending_resolutions_.Get(MAX_RESOLUTIONS_PER_CYCLE))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Resolve: ", ToBase64(result.key.second), ": ",
                    result.promised.uri());
    Uri uri;
    if (result.promised.IsDirectlyConnectable())
    {
      identity_cache_.Update(result.key.second, result.promised);
      muddle_.AddPeer(result.promised);
    }
    else
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,
                      "Discarding resolution for peer: ", ToBase64(result.key.second));
    }
  }

  // process all additional peer requests
  Uri uri;
  for (auto const &address : new_peers)
  {
    bool resolve = true;

    // once the identity has been resolved it can be added as a peer
    if (identity_cache_.Lookup(address, uri) && uri.IsDirectlyConnectable())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Add peer: ", ToBase64(address));
      muddle_.AddPeer(uri);
      resolve = false;
    }

    // if we need to resolve this address
    if (resolve)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Resolve Peer: ", ToBase64(address));
      for (const auto &addr : active_addresses)
      {
        auto key = std::make_pair(addr, address);
        if (pending_resolutions_.IsInFlight(key))
        {
          continue;
        }
        auto prom = network::PromiseOf<Uri>(
            client_.CallSpecificAddress(addr, RPC_P2P_RESOLVER, ResolverProtocol::QUERY, address));
        FETCH_LOG_INFO(LOGGING_NAME, "Resolve Peer: ", ToBase64(address),
                       ", promise id=", prom.id());
        pending_resolutions_.Add(key, prom);
      }
    }
  }

  auto num_of_active_cons = active_addresses.size();
  // dropping peers
  if (num_of_active_cons > min_peers_)
  {
    for (auto const &address : dropped_peers)
    {
      if (identity_cache_.Lookup(address, uri) && uri.IsDirectlyConnectable())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Drop peer: ", ToBase64(address), " -> ", uri.uri());

        muddle_.DropPeer(uri);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to drop peer: ", ToBase64(address));
      }
    }
  }
  for (auto const &address : blacklisted_peers_)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Blacklisting: ", ToBase64(address));
    muddle_.Blacklist(address);
  }
}

void P2PService::UpdateManifests(AddressSet const &active_addresses)
{
  // determine which of the nodes that we are talking too, require an update. This might be
  // because we haven't seen this address before or the information is stale. In either case we need
  // to request an update.
  auto const all_manifest_update_addresses = manifest_cache_.GetUpdatesNeeded(active_addresses);

  // in order to prevent duplicating requests, filter the initial list to only the ones that we have
  // not already requested this information.
  auto const new_manifest_update_addresses =
      outstanding_manifests_.FilterOutInFlight(all_manifest_update_addresses);

  // from the remaining set of addresses schedule a manifest request
  for (auto const &address : new_manifest_update_addresses)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Requesting manifest from: ", ToBase64(address));

    // make the RPC call
    auto prom = network::PromiseOf<network::Manifest>(
        client_.CallSpecificAddress(address, RPC_P2P_RESOLVER, ResolverProtocol::GET_MANIFEST));

    // store the request in our processing queue
    outstanding_manifests_.Add(address, prom);
  }

  // scan through the existing set of outstanding RPC promises and evaluate all the completed items
  outstanding_manifests_.Resolve();

  // process through the completed responses
  auto const manifest_updates = outstanding_manifests_.Get(20);

  for (auto const &result : manifest_updates)
  {
    auto const &address  = result.key;
    auto const &manifest = result.promised;

    // update the manifest cache with the information
    manifest_cache_.ProvideUpdate(address, manifest, 120);

    // distribute the updated manifest at a later point
    DistributeUpdatedManifest(address);
  }
}

void P2PService::DistributeUpdatedManifest(Address const &address)
{
  Manifest manifest;

  if (manifest_cache_.Get(address, manifest))
  {
    local_services_.DistributeManifest(manifest);

    Refresh();
  }
}

void P2PService::Refresh()
{
  local_services_.Refresh();
}

network::Manifest P2PService::GetLocalManifest()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "P2PService::GetLocalManifest", manifest_.ToString());
  return manifest_;
}

P2PService::AddressSet P2PService::GetRandomGoodPeers()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "GetRandomGoodPeers...");

  AddressSet const result = trust_system_.GetRandomPeers(20, 0.0);

  FETCH_LOG_DEBUG(LOGGING_NAME, "GetRandomGoodPeers...num: ", result.size());

  return result;
}

void P2PService::SetLocalManifest(Manifest const &manifest)
{
  manifest_ = manifest;
  local_services_.MakeFromManifest(manifest_);

  thread_pool_->Post([this]() { this->Refresh(); });
}

}  // namespace p2p
}  // namespace fetch
