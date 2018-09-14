#pragma once

#include "core/service_ids.hpp"
#include "network/details/thread_pool.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/muddle/muddle.hpp"
#include "network/peer.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/p2pservice/p2p_resolver.hpp"
#include "network/p2pservice/p2p_resolver_protocol.hpp"
#include "network/p2pservice/p2p_lane_management.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"
#include "network/p2pservice/manifest.hpp"
#include "network/p2pservice/p2p_managed_local_service.hpp"
#include "network/p2pservice/p2p_managed_local_services.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"
#include "network/p2pservice/p2p_remote_manifest_cache.hpp"
#include "network/muddle/rpc/client.hpp"

namespace fetch {
namespace ledger { class LaneRemoteControl; }
namespace p2p {

class P2PService2
{
public:
  using NetworkManager = network::NetworkManager;
  using Muddle = muddle::Muddle;
  using PortList = Muddle::PortList;
  using PeerList = Muddle::PeerList;
  using RpcServer = muddle::rpc::Server;
  using ThreadPool = network::ThreadPool;
  using CertificatePtr = Muddle::CertificatePtr;
  using MuddleEndpoint = muddle::MuddleEndpoint;
  using Identity = crypto::Identity;
  using Uri   = network::Uri;
  using Address   = Resolver::Address;
  using TrustInterface = P2PTrustInterface<Identity>;
  using LaneRemoteControl = ledger::LaneRemoteControl;
  using LaneRemoteControlPtr = std::shared_ptr<LaneRemoteControl>;
  using LaneRemoteControls = std::vector<LaneRemoteControlPtr>;
  using Manifest = network::Manifest;
  using Client = muddle::rpc::Client;
  using PromisedManifests = std::map<Identity, network::PromiseOf<network::Manifest>>;
  using ServiceType = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;

  using RequestingManifests = network::RequestingQueueOf<Identity, Manifest>;
  using RequestingPeerlists = network::RequestingQueueOf<Identity, Uri>;

  enum
  {
    PROTOCOL_RESOLVER = 1
  };
    static constexpr char const *LOGGING_NAME = "P2PService2";

  // Construction / Destruction
  P2PService2(Muddle &muddle, LaneManagement &lane_management);
  ~P2PService2() = default;

  void Start(PeerList const & initial_peer_list, Uri my_uri);
  void Stop();

  void SetPeerGoals(uint32_t min, uint32_t max);

  Identity const &identity() const { return muddle_ . identity(); }
  MuddleEndpoint& AsEndpoint() { return muddle_ . AsEndpoint(); }

  void PeerIdentificationSucceeded(const Uri &peer, const Identity &identity);
  void PeerIdentificationFailed   (const Uri &peer);
  void PeerTrustEvent(const Identity &          identity
                      , P2PTrustFeedbackSubject subject
                      , P2PTrustFeedbackQuality quality);

  void SetLocalManifest(Manifest &&manifest);
  Manifest GetLocalManifest();
  std::vector<Uri> GetRandomGoodPeers();

  void WorkCycle();

private:
  void DistributeUpdatedManifest(Identity identity_of_updated_peer);
  void Refresh();

  std::map<Identity, Uri> identity_to_uri_;

  Muddle  &muddle_;
  MuddleEndpoint &muddle_ep_;
  ThreadPool  thread_pool_ = network::MakeThreadPool(10);
  RpcServer rpc_server_{muddle_.AsEndpoint(), SERVICE_P2P, CHANNEL_RPC};

  LaneManagement &lane_management_;

  // address resolution service
  Resolver resolver_;
  ResolverProtocol resolver_proto_;

  std::shared_ptr<TrustInterface> trust_system;

  Uri my_uri_;

  Client client_;
  Manifest manifest_;
  std::map<Identity, Manifest> discovered_peers_;

  P2PManagedLocalServices local_services_;

  RequestingManifests outstanding_manifests_;
  RequestingPeerlists outstanding_peerlists_;

  P2PRemoteManifestCache manifest_cache_;
  std::list<network::Uri> possibles_; // addresses we might use in the future.

  uint32_t min_peers, max_peers;
};

} // namespace p2p
} // namespace fetch
