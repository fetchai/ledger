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

#include "direct_message_service.hpp"
#include "discovery_service.hpp"
#include "muddle_register.hpp"
#include "peer_list.hpp"
#include "peer_selector.hpp"

#include "core/containers/set_difference.hpp"
#include "core/reactor.hpp"
#include "core/service_ids.hpp"

namespace fetch {
namespace muddle {

static constexpr std::size_t MINIMUM_PEERS = 3;
static constexpr char const *LOGGING_NAME  = "PeerSelector";

PeerSelector::PeerSelector(Duration const &interval, core::Reactor &reactor,
                           MuddleRegister const &reg, PeerConnectionList &connections,
                           MuddleEndpoint &endpoint)
  : core::PeriodicRunnable(interval)
  , reactor_{reactor}
  , connections_{connections}
  , register_{reg}
  , endpoint_{endpoint}
  , rpc_client_{"PeerSelect", endpoint_, SERVICE_MUDDLE, CHANNEL_RPC}
{}

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

void PeerSelector::Periodically()
{
  FETCH_LOCK(lock_);

  // get the set of addresses to which we want to connect to
  auto const currently_connected_peers = register_.GetCurrentAddressSet();
  auto const current_outgoing_peers    = register_.GetOutgoingAddressSet();
  auto const outstanding_peers         = desired_addresses_ - currently_connected_peers;
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
        // lookup the connection from its address
        auto conn = register_.LookupConnection(address).lock();
        if (conn)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Dropping Address: ", address.ToBase64());

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
  auto const unresolved_addresses = (addresses - peers_info_) - pending_resolutions_;

  for (auto const &address : unresolved_addresses)
  {
    // make the call to the remote service
    auto promise = rpc_client_.CallSpecificAddress(address, RPC_MUDDLE_DISCOVERY,
                                                   DiscoveryService::CONNECTION_INFORMATION);

    // wrap the promise is a task
    auto task = std::make_shared<PromiseTask>(
        std::move(promise),
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

    FETCH_LOG_INFO(LOGGING_NAME, "Successful resolution for ", address.ToBase64());

    // create the new entry and populate
    auto &metadata = peers_info_[address];
    for (auto const &peer_address : peer_addresses)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "- Candidate: ", peer_address.ToString());
      metadata.peer_data.emplace_back(peer_address);
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to resolve address for: ", address.ToBase64(),
                   " code: ", int(promise->state()));
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
            FETCH_LOG_CRITICAL(LOGGING_NAME, "Marking ", current_uri.ToString(), " as unreachable");
            current_peer.unreachable = true;
          }
        }

        if (!current_peer.unreachable)
        {
          FETCH_LOG_TRACE(LOGGING_NAME, "Mapped ", address.ToBase64(), " to ",
                          current_uri.ToString());

          uris.emplace(std::move(current_uri));
          break;  // ensure we do not populate multiple entries for a single address
        }
      }
    }
  }

  return uris;
}

}  // namespace muddle
}  // namespace fetch
