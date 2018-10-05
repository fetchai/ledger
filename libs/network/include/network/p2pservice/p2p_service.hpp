#pragma once
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

#include "core/service_ids.hpp"
#include "network/details/thread_pool.hpp"
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
#include "network/peer.hpp"

#include <unordered_map>

namespace fetch {
namespace ledger {
class LaneRemoteControl;
}
namespace p2p {

class P2PService
{
public:
  using NetworkManager       = network::NetworkManager;
  using Muddle               = muddle::Muddle;
  using PortList             = Muddle::PortList;
  using UriList              = Muddle::UriList;
  using RpcServer            = muddle::rpc::Server;
  using ThreadPool           = network::ThreadPool;
  using CertificatePtr       = Muddle::CertificatePtr;
  using MuddleEndpoint       = muddle::MuddleEndpoint;
  using Identity             = crypto::Identity;
  using Uri                  = network::Uri;
  using Address              = Resolver::Address;
  using TrustInterface       = P2PTrustInterface<Address>;
  using LaneRemoteControl    = ledger::LaneRemoteControl;
  using LaneRemoteControlPtr = std::shared_ptr<LaneRemoteControl>;
  using LaneRemoteControls   = std::vector<LaneRemoteControlPtr>;
  using Manifest             = network::Manifest;
  using Client               = muddle::rpc::Client;
  using PromisedManifests    = std::map<Identity, network::PromiseOf<network::Manifest>>;
  using ServiceType          = network::ServiceType;
  using ServiceIdentifier    = network::ServiceIdentifier;
  using ConnectionState      = muddle::PeerConnectionList::ConnectionState;
  using UriSet               = std::unordered_set<Uri>;
  using AddressSet           = std::unordered_set<Address>;
  using ConnectionMap        = muddle::Muddle::ConnectionMap;
  using FutureTimepoint      = network::FutureTimepoint;

  static constexpr char const *LOGGING_NAME = "P2PService";

  // Construction / Destruction
  P2PService(Muddle &muddle, LaneManagement &lane_management, TrustInterface &trust);
  ~P2PService() = default;

  void Start(UriList const &initial_peer_list, Uri const &my_uri);
  void Stop();

  void SetPeerGoals(uint32_t min, uint32_t max);

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

private:
  using RequestingManifests = network::RequestingQueueOf<Address, Manifest>;
  using RequestingPeerlists = network::RequestingQueueOf<Address, AddressSet>;
  using RequestingUris      = network::RequestingQueueOf<Address, Uri>;

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

  ThreadPool thread_pool_ = network::MakeThreadPool(1);
  RpcServer  rpc_server_{muddle_.AsEndpoint(), SERVICE_P2P, CHANNEL_RPC};

  // Node Information
  Address const address_;   ///< The address / public key of the current node
  Uri           my_uri_;    ///< The public addresss associated with this node
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
  RequestingUris      pending_resolutions_;    ///< The queue of outstaing resolutions
  AddressSet desired_peers_;  ///< The desired set of addresses that we want to have connections to
  ManifestCache manifest_cache_;  ///< The cache of manifests of the peers to which we are connected
  P2PManagedLocalServices local_services_;
  std::size_t             work_cycle_count_ = 0;  ///< Counter to manage periodic task intervals
  ///@}

  uint32_t min_peers_ = 2;
  uint32_t max_peers_ = 3;
};

}  // namespace p2p
}  // namespace fetch
