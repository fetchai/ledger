#include "network/p2pservice/p2p_service2.hpp"
#include "network/p2pservice/p2ptrust.hpp"
#include "network/p2pservice/manifest.hpp"

namespace fetch {
namespace p2p {

P2PService2::P2PService2(Muddle &muddle, LaneManagement &lane_management)
  : muddle_(muddle)
  , muddle_ep_(muddle . AsEndpoint())
  , lane_management_{lane_management}
  , resolver_proto_{resolver_, *this}
  , client_(muddle_ep_, Muddle::Address(), SERVICE_P2P, CHANNEL_RPC)
  , local_services_(lane_management_)
{
  // register the services with the rpc server
  rpc_server_.Add(PROTOCOL_RESOLVER, &resolver_proto_);
  trust_system = std::make_shared<P2PTrust<Identity>>();

  this -> min_peers = 4;
  this -> max_peers = 8;
}

void P2PService2::Start(P2PService2::PeerList const &initial_peer_list, P2PService2::Uri my_uri)
{
  for(auto &peer : initial_peer_list)
  {
    possibles_.push_back(peer . ToUri());
  }

  thread_pool_ -> SetInterval(1000);
  thread_pool_ -> Start();
  thread_pool_ -> PostIdle([this](){ this -> WorkCycle(); });

  my_uri_ = my_uri;
}

void P2PService2::Stop()
{
  thread_pool_ -> clear();
  thread_pool_ -> Stop();
}

void P2PService2::WorkCycle()
{
  FETCH_LOG_DEBUG(LOGGING_NAME,"P2PService2::WorkCycle");

  // see how many peers we have.

  std::set<Uri> used;
  std::list<Identity> connected_peers;

  auto connections = muddle_.GetConnections(); // address/uri/state tuples.
  FETCH_LOG_DEBUG(LOGGING_NAME,"P2PService2::WorkCycle: Conncount = ", connections.size());
  for(auto const &connection : connections)
  {
    // need to do some filtering based on channels and services and protocols.

    // decompose the tuple
    auto const &address = std::get<0>(connection);
    auto const &uri     = std::get<1>(connection);
    Identity identity{"", address};

    FETCH_LOG_DEBUG(LOGGING_NAME,"P2PService2::WorkCycle: Conn:", ToHex(addr), " / ", uri.ToString(), " / ", state);

    if (uri.length() > 0)
    {
      identity_to_uri_[identity] = uri;
    }

    if (trust_system)
    {
      if (!trust_system -> IsPeerKnown(identity))
      {
        trust_system -> AddFeedback(identity, PEER, NEW_INFORMATION);
      }
    }

    used.insert(uri);
    connected_peers.push_back(identity);
  }

  // not enough, schedule some connects.
  while((connections.size() < min_peers) && (possibles_.size() > 0))
  {
    auto next = possibles_ . front();
    possibles_.pop_front();
    FETCH_LOG_DEBUG(LOGGING_NAME,"P2PService2::WorkCycle: PULLED ", next.ToString());

    if (next.GetProtocol() == "tcp")
    {
      auto s = next.GetRemainder();
      FETCH_LOG_DEBUG(LOGGING_NAME,"Converting: ", next.ToString(), " -> ", s);

      auto nextp = next.AsPeer();
      switch (muddle_.useClients().GetStateForPeer(nextp))
      {
      case muddle::PeerConnectionList::UNKNOWN:
        FETCH_LOG_DEBUG(LOGGING_NAME,"P2PService2::WorkCycle: AddPeer  ", nextp.ToString());
        muddle_ . AddPeer(nextp);
        break;
      case muddle::PeerConnectionList::CONNECTED:
        FETCH_LOG_DEBUG(LOGGING_NAME,"P2PService2::WorkCycle: Considered, but in use:  ", next.ToString());
        break;
      case muddle::PeerConnectionList::TRYING:
        FETCH_LOG_DEBUG(LOGGING_NAME,"P2PService2::WorkCycle: Considered, but being tried:  ", next.ToString());
        break;
      case muddle::PeerConnectionList::BACKOFF:
      default:
        FETCH_LOG_DEBUG(LOGGING_NAME,"P2PService2::WorkCycle: Considered, but in backoff:  ", next.ToString());
        break;
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: No Go on ", next.ToString(), ">>", next.GetProtocol());
    }
  }

  // too many? schedule some kickoffs.

  // gte more peers if we need them..

  if (possibles_.size() < max_peers)
  {
    auto peerlist_updates_needed_and_not_in_flight = outstanding_peerlists_ . FilterOutInflight( connected_peers );
    if (peerlist_updates_needed_and_not_in_flight . size() == 0)
    {
      FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: outstanding left no peers");
    }
    for( auto& identity : peerlist_updates_needed_and_not_in_flight)
    {
      FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: get peers from ", ToHex(identity.identifier()));
      client_ . SetAddress(identity.identifier());
      auto prom = network::PromiseOf<std::vector<network::Uri>>(
        client_ . Call(PROTOCOL_RESOLVER, ResolverProtocol::GET_RANDOM_GOOD_PEERS)
      );
      outstanding_peerlists_ . Add( identity, prom );
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: possibles = ", possibles_.size());
  }

  outstanding_peerlists_ . Resolve();

  FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: processing outstanding.");
  while(!outstanding_peerlists_ . empty())
  {
    std::vector<RequestingPeerlists::OutputType> outputs;
    outstanding_peerlists_ . Get(outputs, 20);

    for(auto &it : outputs)
    {
      auto new_peer = it . second;
      FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: possible new peer= ", new_peer.ToString());
      //auto new_identity = it . first; // needed for trust scoring later...

      if (my_uri_ != new_peer)
      {
        FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: remembering possible new peer= ", new_peer.ToString());
        possibles_ . push_back( new_peer );
      }
    }
  }
  FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: processing outstanding DONE.");

  // handle manifest updates.=
  auto manifest_updates_needed_from = manifest_cache_.GetUpdatesNeeded( connected_peers );
  auto manifest_updates_needed_and_not_in_flight = outstanding_manifests_ . FilterOutInflight( manifest_updates_needed_from );

  for( auto& identity : manifest_updates_needed_and_not_in_flight )
  {
    FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: get manifest from ", ToHex(identity.identifier()));
    client_ . SetAddress(identity.identifier());
    auto prom = network::PromiseOf<network::Manifest>(
      client_ . Call(PROTOCOL_RESOLVER, ResolverProtocol::GET_MANIFEST)
    );
    outstanding_manifests_ . Add( identity, prom );
  }

  outstanding_manifests_ . Resolve();

  while(!outstanding_manifests_ . empty())
  {
    std::vector<RequestingManifests::OutputType> outputs;
    outstanding_manifests_ . Get(outputs, 20);
    for(auto &it : outputs)
    {
      auto new_manifest = it . second;
      auto new_identity = it . first;

      manifest_cache_ . ProvideUpdate(new_identity, new_manifest, 10);
      auto cb = [ this, new_identity ](){ this -> DistributeUpdatedManifest(new_identity); };
      thread_pool_ -> Post( cb );
    }
  }

  FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: COMPLETE.");
}

void P2PService2::DistributeUpdatedManifest(Identity identity_of_updated_peer)
{
  auto possible_manifest = manifest_cache_ . Get( identity_of_updated_peer );
  if (!possible_manifest.first)
  {
    return;
  }
  local_services_ . DistributeManifest(possible_manifest.second);
  thread_pool_ -> Post( [this](){
      this -> Refresh();
    });
}

void P2PService2::Refresh()
{
  local_services_ . Refresh();
}

network::Manifest P2PService2::GetLocalManifest()
{
  FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::GetLocalManifest", manifest_ . ToString());
  return manifest_;
}

std::vector<P2PService2::Uri> P2PService2::GetRandomGoodPeers()
{
  std::vector<P2PService2::Uri> result;
  if (trust_system)
  {
    auto peers = trust_system -> GetRandomPeers(20, 0.0);
    for(auto &peer : peers)
    {
      auto iter = identity_to_uri_ . find(peer);
      if (iter != identity_to_uri_ . end())
      {
        result . push_back(iter -> second);
      }
    }
  }
  return result;
}

void P2PService2::PeerIdentificationSucceeded(const P2PService2::Uri &peer, const P2PService2::Identity &identity)
{
}

void P2PService2::PeerIdentificationFailed   (const P2PService2::Uri &peer)
{
}

void P2PService2::PeerTrustEvent(const Identity &          identity
                                 , P2PTrustFeedbackSubject subject
                                 , P2PTrustFeedbackQuality quality)
{
}

void P2PService2::SetPeerGoals(uint32_t min, uint32_t max)
{
  this -> min_peers = min;
  this -> max_peers = max;
}

void P2PService2::SetLocalManifest(Manifest &&manifest)
{
  manifest_ = std::move(manifest);
  local_services_ . MakeFromManifest(manifest_);

  thread_pool_ -> Post([this](){ this -> Refresh(); });
}



} // namespace p2p
} // namespace fetc
