#pragma once
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

#include "core/service_ids.hpp"
#include "network/details/thread_pool.hpp"
#include "network/generics/promise_of.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/p2pservice/identity_cache.hpp"
#include "network/p2pservice/manifest.hpp"
#include "network/p2pservice/p2p_lane_management.hpp"
#include "network/p2pservice/p2p_managed_local_service.hpp"
#include "network/p2pservice/p2p_managed_local_services.hpp"
#include "network/p2pservice/p2p_remote_manifest_cache.hpp"
#include "network/p2pservice/p2p_resolver.hpp"
#include "network/p2pservice/p2p_resolver_protocol.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"
#include "network/uri.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fetch {
namespace p2p {

class LaneManagement;

class P2PService
{
public:
  using NetworkManager    = network::NetworkManager;
  using Muddle            = muddle::Muddle;
  using PortList          = Muddle::PortList;
  using UriList           = Muddle::UriList;
  using RpcServer         = muddle::rpc::Server;
  using ThreadPool        = network::ThreadPool;
  using CertificatePtr    = Muddle::CertificatePtr;
  using MuddleEndpoint    = muddle::MuddleEndpoint;
  using Identity          = crypto::Identity;
  using Uri               = network::Uri;
  using Address           = Resolver::Address;
  using TrustInterface    = P2PTrustInterface<Address>;
  using Manifest          = network::Manifest;
  using Client            = muddle::rpc::Client;
  using PromisedManifests = std::map<Identity, network::PromiseOf<network::Manifest>>;
  using ServiceType       = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;
  using ConnectionState   = muddle::PeerConnectionList::ConnectionState;
  using UriSet            = std::unordered_set<Uri>;
  using AddressSet        = std::unordered_set<Address>;
  using ConnectionMap     = muddle::Muddle::ConnectionMap;
  using FutureTimepoint   = core::FutureTimepoint;
  using PeerTrust         = TrustInterface::PeerTrust;

  static constexpr char const *LOGGING_NAME = "P2PService";

  // Construction / Destruction
  P2PService(Muddle &muddle, LaneManagement &lane_management, TrustInterface &trust,
             std::size_t max_peers, std::size_t transient_peers, uint32_t process_cycle_ms);
  ~P2PService() = default;

  void Start(UriList const &initial_peer_list);
  void Stop();

  Identity const &identity() const
  {
    return muddle_.identity();
  }
  MuddleEndpoint &AsEndpoint()
  {
    return muddle_.AsEndpoint();
  }

  void       SetLocalManifest(Manifest const &manifest);
  Manifest   GetLocalManifest();
  AddressSet GetRandomGoodPeers();

  Uri GetNodeUri()  // can't be const due to RPC protocol
  {
    return my_uri_;  // TODO(EJF): Technically a race here, however, the assumption is that this
                     // value will not change
  }

  IdentityCache const &identity_cache() const
  {
    return identity_cache_;
  }

  bool IsDesired(Address const &address);

private:
  struct PairHash
  {
  public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const
    {
      return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
  };

  using RequestingManifests = network::RequestingQueueOf<Address, Manifest>;
  using RequestingPeerlists = network::RequestingQueueOf<Address, AddressSet>;
  using RequestingUris      = network::RequestingQueueOf<std::pair<Address, Address>, Uri,
                                                    network::PromiseOf<Uri>, PairHash>;

  /// @name Work Cycle
  /// @{
  void WorkCycle();
  void GetConnectionStatus(ConnectionMap &active_connections, AddressSet &active_addresses);
  void UpdateTrustStatus(ConnectionMap const &active_connections);
  void PeerDiscovery(AddressSet const &active_addresses);
  void RenewDesiredPeers(AddressSet const &active_addresses);
  void UpdateMuddlePeers(AddressSet const &active_addresses);
  void UpdateManifests(AddressSet const &active_addresses);
  /// @}

  void DistributeUpdatedManifest(Address const &address);
  void Refresh();

  // System components
  Muddle &        muddle_;           ///< The reference to the muddle network stack
  MuddleEndpoint &muddle_ep_;        ///< The bridge to the muddle endpoint
  LaneManagement &lane_management_;  ///< The lane management service
  TrustInterface &trust_system_;     ///< The trust system

  ThreadPool thread_pool_ = network::MakeThreadPool(1, "CORE");
  RpcServer  rpc_server_{muddle_.AsEndpoint(), SERVICE_P2P, CHANNEL_RPC};

  // Node Information
  Address const address_;   ///< The address / public key of the current node
  Uri           my_uri_;    ///< The public address associated with this node
  Manifest      manifest_;  ///< The manifest associated with this address

  // identity cache
  IdentityCache identity_cache_;  ///< The cache of identity vs. muddle a

  // address resolution service
  Resolver         resolver_;        ///< The resolver
  ResolverProtocol resolver_proto_;  ///< The protocol for the resolver

  // Work Cycle specific data
  /// @{
  Client              client_;                 ///< The RPC client adapter
  RequestingManifests outstanding_manifests_;  ///< The queue of outstanding promises for manifests
  RequestingPeerlists pending_peer_lists_;     ///< The queue of outstanding peer lists
  RequestingUris      pending_resolutions_;    ///< The queue of outstanding resolutions
  AddressSet desired_peers_;  ///< The desired set of addresses that we want to have connections to
  AddressSet blacklisted_peers_;  ///< The set of addresses that we will not have connections to
  ManifestCache manifest_cache_;  ///< The cache of manifests of the peers to which we are connected
  P2PManagedLocalServices local_services_;
  ///@}

  std::size_t min_peers_ = 2;
  std::size_t max_peers_;
  std::size_t transient_peers_;

  std::chrono::milliseconds peer_update_cycle_ms_;

  FutureTimepoint process_future_timepoint_;

  std::chrono::milliseconds manifest_update_cycle_ms_;
  FutureTimepoint           manifests_next_update_timepoint_;

  static constexpr std::size_t MAX_PEERS_PER_CYCLE = 32;
};

}  // namespace p2p
}  // namespace fetch
