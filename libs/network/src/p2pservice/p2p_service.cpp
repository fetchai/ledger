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
#include <random>

namespace fetch {
namespace p2p {

class BlockCatchUpService
{
public:
  using PromiseState    = fetch::service::PromiseState;
  using Promise         = service::Promise;
  using Muddle          = muddle::Muddle;
  using Address         = Resolver::Address;
  using AddressSet      = std::unordered_set<Address>;
  using Client          = muddle::rpc::Client;
  using Mutex           = mutex::Mutex;
  using ChainRpcPtr     = std::weak_ptr<ledger::MainChainRpcService>;
  using Block           = chain::MainChain::BlockType;
  using BlockQueue      = network::RequestingQueueOf<Address, std::vector<Block>>;
  using Uri             = network::Uri;
  using UriSet          = std::unordered_set<Uri>;
  using TrustInterface  = P2PTrustInterface<Address>;

  BlockCatchUpService(Muddle &muddle, TrustInterface &trust)
   : muddle_(muddle)
   , trust_(trust)
   , random_engine_(std::random_device()())
   , distribution_()
   {
    next.Set(std::chrono::milliseconds(0));
  }

  void AddChainRpc(ChainRpcPtr&& chain_rpc)
  {
    chain_rpc_ = std::move(chain_rpc);
  }

  void WorkCycle()
  {
    AddressSet callable_peers;
    if (next.IsDue())
    {
      FETCH_LOG_WARN("BlockCatchUpService", "New_peers_.size=", new_peers_.size());
        FETCH_LOCK(mutex_);
        std::vector<double> ws;
        std::vector<UriSet::iterator> its;
        double sum_w = 0.;
        for (auto it = new_peers_.begin(); it != new_peers_.end(); ++it) {
          Address address;
          if (muddle_.UriToDirectAddress(*it, address)) {
            callable_peers.insert(address);
            auto w = trust_.GetPeerUsefulness(address);
            ws.push_back(w);
            its.push_back(it);
            sum_w += w;
            FETCH_LOG_INFO("BlockCatchUpService", "Adding connected address: ", ToBase64(address));
          }
        }
        if (new_peers_.size()>KEEP_PEERS) {
          for(std::size_t i = 0; i<ws.size();++i)
          {
            if (distribution_(random_engine_)>ws[i]/sum_w)
            {
              new_peers_.erase(its[i]);
            }
          }
        }
      next.Set(TIMEOUT);
      FETCH_LOG_WARN("BlockCatchUpService", "End of promise creation");

    }
    auto chain_rpc_ptr = chain_rpc_.lock();
    if (chain_rpc_ptr!=nullptr)
    {
      for(auto &address : block_promises_.FilterOutInFlight(callable_peers))
      {
        auto prom = chain_rpc_ptr->GetLatestBlockFromAddress(address);
        block_promises_.Add(address, prom);
        FETCH_LOG_WARN("BlockCatchUpService", "Sending GetLatestBlock request to address: ", ToBase64(address));
      }
      block_promises_.Resolve();
      for(auto &res : block_promises_.Get(MAX_RESOLUTIONS_PER_CYCLE))
      {
        if (res.promised.size()>0)
        {
          auto block = res.promised[0];
          block.UpdateDigest();
          if (block.hash()!=last_hash_)
          {
            FETCH_LOG_WARN("BlockCatchUpService", "Got new block from: ", ToBase64(res.key), ", block hash: ", ToBase64(block.hash()));
            last_hash_ = block.hash();
            chain_rpc_ptr->OnNewLatestBlock(res.key, block);
          }
        }
      }
    }
    else
    {
      FETCH_LOG_ERROR("BlockCatchUpService", "No chain_rpc_pointer!");
    }
  }

  void AddUri(Uri const &uri)
  {
    FETCH_LOCK(mutex_);
    new_peers_.insert(uri);
  }
  void RemoveUri(Uri const &uri)
  {
    FETCH_LOCK(mutex_);
    new_peers_.erase(uri);
  }
private:
  Muddle &muddle_;
  TrustInterface &trust_;
  ChainRpcPtr chain_rpc_;
  UriSet new_peers_;
  BlockQueue block_promises_;
  byte_array::ConstByteArray last_hash_;
  network::FutureTimepoint next;
  std::chrono::milliseconds const TIMEOUT{1000};
  std::mt19937 random_engine_;
  std::exponential_distribution<double> distribution_;
  static constexpr std::size_t const KEEP_PEERS = 3;
  static constexpr std::size_t const MAX_RESOLUTIONS_PER_CYCLE = 32;
  Mutex mutex_{__LINE__, __FILE__};
};

P2PService::P2PService(Muddle &muddle, LaneManagement &lane_management, TrustInterface &trust,
                       std::size_t max_peers, std::size_t transient_peers,
                       uint32_t process_cycle_ms)
  : muddle_(muddle)
  , muddle_ep_(muddle.AsEndpoint())
  , lane_management_{lane_management}
  , trust_system_{trust}
  , address_(muddle.identity().identifier())
  , identity_cache_{}
  , resolver_{identity_cache_}
  , resolver_proto_{resolver_, *this}
  , client_(muddle_ep_, Muddle::Address(), SERVICE_P2P, CHANNEL_RPC)
  , local_services_(lane_management_)
  , max_peers_(max_peers)
  , transient_peers_(transient_peers)
  , process_cycle_ms_(1000)
  , peer_update_cycle_ms_(process_cycle_ms)
  , latest_block_sync_{std::make_shared<BlockCatchUpService>(muddle_, trust_system_)}
{
  // register the services with the rpc server
  rpc_server_.Add(RPC_P2P_RESOLVER, &resolver_proto_);
  process_future_timepoint_.Set(std::chrono::milliseconds(0));
}

void P2PService::AddMainChainRpcService(std::shared_ptr<ledger::MainChainRpcService> ptr)
{
  latest_block_sync_->AddChainRpc(std::weak_ptr<ledger::MainChainRpcService>(ptr));
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
    latest_block_sync_->AddUri(uri);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Establishing CORE Service on tcp://127.0.0.1:", "??",
                 " ID: ", byte_array::ToBase64(muddle_.identity().identifier()));

  thread_pool_->SetIdleInterval(process_cycle_ms_);
  thread_pool_->Start();
  if (process_cycle_ms_ > 0)
  {
    thread_pool_->PostIdle([this]() { WorkCycle(); });
  }
}

void P2PService::Stop()
{
  thread_pool_->Clear();
  thread_pool_->Stop();
}

void P2PService::WorkCycle()
{
  latest_block_sync_->WorkCycle();
  if (process_future_timepoint_.IsDue())
  {
  process_future_timepoint_.Set(peer_update_cycle_ms_);
  // get the summary of all the current connections
  ConnectionMap active_connections;
  AddressSet    active_addresses;
  FETCH_LOG_WARN(LOGGING_NAME, "Before GetConnectionStatus");
  GetConnectionStatus(active_connections, active_addresses);
  FETCH_LOG_WARN(LOGGING_NAME, "End GetConnectionStatus");


  // update our identity cache (address -> uri mapping)
    FETCH_LOG_WARN(LOGGING_NAME, "Before identity_cache_.Update");
    identity_cache_.Update(active_connections);
    FETCH_LOG_WARN(LOGGING_NAME, "End identity_cache_.Update");


    // update the trust system with current connection information
    FETCH_LOG_WARN(LOGGING_NAME, "Before UpdateTrustStatus");
    UpdateTrustStatus(active_connections);
    FETCH_LOG_WARN(LOGGING_NAME, "End UpdateTrustStatus");

    // discover new good peers on the network
    FETCH_LOG_WARN(LOGGING_NAME, "Before PeerDiscovery");
    PeerDiscovery(active_addresses);
    FETCH_LOG_WARN(LOGGING_NAME, "End PeerDiscovery");


    // make the decisions about which peers are desired and which ones we now need to drop
    FETCH_LOG_WARN(LOGGING_NAME, "Before RenewDesiredPeers");
    RenewDesiredPeers(active_addresses);
    FETCH_LOG_WARN(LOGGING_NAME, "End RenewDesiredPeers");


    // perform connections updates and drops based on previous step
    FETCH_LOG_WARN(LOGGING_NAME, "Before UpdateMuddlePeers");
    UpdateMuddlePeers(active_addresses);
    FETCH_LOG_WARN(LOGGING_NAME, "End UpdateMuddlePeers");


    // collect up manifests from connected peers
    FETCH_LOG_WARN(LOGGING_NAME, "Before UpdateManifests");
    UpdateManifests(active_addresses);
    FETCH_LOG_WARN(LOGGING_NAME, "End UpdateManifests");

    // increment the work cycle counter (used for scheduling of periodic events)
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
  FETCH_LOCK(desired_peers_mutex_);
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
        trust_system_.AddObjectFeedback(address, TrustSubject::PEER, TrustQuality::LIED);
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
          trust_system_.AddFeedback(from, new_address, TrustSubject::PEER,
                                    TrustQuality::NEW_INFORMATION);
        }
      }
    }
  }
}

bool P2PService::IsDesired(Address const &address) const
{
  FETCH_LOCK(desired_peers_mutex_);
  return desired_peers_.find(address) != desired_peers_.end();
}

bool P2PService::IsExperimental(Address const &address) const
{
  FETCH_LOCK(desired_peers_mutex_);
  return experimental_peers_.find(address) != experimental_peers_.end();
}

void P2PService::RenewDesiredPeers(AddressSet const & /*active_addresses*/)
{
  FETCH_LOCK(desired_peers_mutex_);

  PeerTrusts peerTrusts = trust_system_.GetPeersAndTrusts();

  auto erase_untrusted_peers = std::remove_if(
      peerTrusts.begin(), peerTrusts.end(),
      [](PeerTrust const &x) {
        return x.trust <= 0.0;
      });
  peerTrusts.erase(erase_untrusted_peers, peerTrusts.end());

  auto erase_unknown_peers = std::remove_if(
      peerTrusts.begin(), peerTrusts.end(),
      [this](PeerTrust const &x) {
        Uri uri;
        return !(identity_cache_.Lookup(x.address, uri) && uri.IsDirectlyConnectable());
      });
  peerTrusts.erase(erase_unknown_peers, peerTrusts.end());

  std::sort(
      peerTrusts.begin(), peerTrusts.end(),
      [](PeerTrust const &a,PeerTrust const &b)
      {
        return a.trust < b.trust;
      });

  desired_peers_.clear();
  experimental_peers_.clear();

  while(
        !peerTrusts.empty()
        &&
        desired_peers_.size() < max_peers_ - transient_peers_
        )
  {
    auto p = peerTrusts.back().address;
    peerTrusts.pop_back();
    desired_peers_.insert(p);
  }

  auto new_experimental_peers = trust_system_.GetRandomPeers(transient_peers_, 0.0);
  for (auto const &p : new_experimental_peers)
  {
    if (desired_peers_.find(p) == desired_peers_.end())
    {
      desired_peers_.insert(p);
      experimental_peers_.insert(p);
    }
  }
}

void P2PService::UpdateMuddlePeers(AddressSet const &active_addresses)
{
  static constexpr std::size_t MAX_RESOLUTIONS_PER_CYCLE = 20;

  AddressSet const outgoing_peers = identity_cache_.FilterOutUnresolved(active_addresses);

  FETCH_LOCK(desired_peers_mutex_);

  AddressSet const new_peers     = desired_peers_ - active_addresses;
  AddressSet const dropped_peers = outgoing_peers - desired_peers_;

  for (auto const &d : desired_peers_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Muddle Update: KEEP: ", ToBase64(d));
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
      latest_block_sync_->AddUri(uri);
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
      latest_block_sync_->AddUri(uri);
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
        latest_block_sync_->RemoveUri(uri);
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

  AddressSet const result =
      trust_system_.GetRandomPeers(20, 0.0);  // TODO(ATTILA): Why is 20 hardcoded?

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
