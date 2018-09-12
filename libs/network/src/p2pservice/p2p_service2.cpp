#include "network/p2pservice/p2p_service2.hpp"
#include "network/p2pservice/p2ptrust.hpp"

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

  counter = 0;

  // create all the remote control instances
}

void P2PService2::Start(P2PService2::PeerList const &initial_peer_list, int my_port_number)
{
  for(auto &peer : initial_peer_list)
  {
    possibles_.push_back(peer . ToUri());
  }

  thread_pool_ -> SetInterval(1000);
  thread_pool_ -> Start();

  port_number = my_port_number;
  thread_pool_ -> PostIdle([this](){ this -> WorkCycle(); });
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
  FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Conncount = ", connections.size());
  for(auto const &connection : connections)
  {
    // need to do some filtering based on channels and services and protocols.

    // decompose the tuple
    auto const &address = std::get<0>(connection);
    auto const &uri     = std::get<1>(connection);
    auto const &state   = std::get<2>(connection);

    FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Conn:", ToBase64(address), " / ", uri.ToString(), " / ", state);

    used.insert(uri);
    connected_peers.push_back(Identity{"", address});
  }
  FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: port_number ", port_number);

  // not enough, schedule some connects.
  while((connections.size() < 1000) && (possibles_.size() > 0))
  {
    auto next = possibles_ . front();
    possibles_.pop_front();
    FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: PULLED ", next.ToString());

    if (next.GetProtocol() == "tcp")
    {
      auto s = next.GetRemainder();
      FETCH_LOG_WARN(LOGGING_NAME,"Converting: ", next.ToString(), " -> ", s);

      auto nextp = next.AsPeer();
      switch (muddle_.useClients().GetStateForPeer(nextp))
      {
      case muddle::PeerConnectionList::UNKNOWN:
        FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: AddPeer  ", nextp.ToString());
        muddle_ . AddPeer(nextp);
        break;
      case muddle::PeerConnectionList::CONNECTED:
        FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Considered, but in use:  ", next.ToString());
        break;
      case muddle::PeerConnectionList::TRYING:
        FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Considered, but being tried:  ", next.ToString());
        break;
      case muddle::PeerConnectionList::BACKOFF:
      default:
        FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Considered, but in backoff:  ", next.ToString());
        break;
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: No Go on ", next.ToString(), ">>", next.GetProtocol());
    }
  }

  if (port_number > 8019)
  {
    counter ++;

    //if (counter<10)
    //{
    //  return;
    //}
  }
  // too many? schedule some kickoffs.

  // handle manifest updates.

  auto manifest_update_needed_from = manifest_cache_.GetUpdatesNeeded( connected_peers );
  for( auto& identity : manifest_update_needed_from)
  {
    if (
        (promised_manifests_ . find(identity) == promised_manifests_.end())
        &&
        (promised_manifests_ . size() < 1)
      )
    {
      FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: get manifest from ", ToHex(identity.identifier()));
      //FETCH_LOG_WARN(LOGGING_NAME,"P2PService2:: Would get manifest from ", ToHex(identity.identifier()));
      client_ . SetAddress(identity.identifier());

      auto prom = network::PromiseOf<network::Manifest>(
        client_ . Call(PROTOCOL_RESOLVER, ResolverProtocol::GET_MANIFEST)
      );
      promised_manifests_.insert(
        std::make_pair(
          identity,
          prom
        )
      );
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: INFLIGHT get manifest from ", ToBase64(identity.identifier()));
    }
  }

  PromisedManifests::iterator it = promised_manifests_.begin();
  while(it != promised_manifests_.end())
  {
    auto status = it -> second.GetState();
    if (status !=  network::PromiseOf<network::Manifest>::State::WAITING)
    {
      if (status == network::PromiseOf<network::Manifest>::State::SUCCESS)
      {
        FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Reading promised manifest from ",
                       it -> second.id(),".",
                       it -> second.GetInnerPromise() -> protocol(),".",
                       it -> second.GetInnerPromise() -> function()
                       );
        try
        {
          FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Reading promised manifest from ", it->second.GetInnerPromise() -> Schmoo());

          auto new_manifest = it->second.Get();
          auto moo_identity = it->first;

          manifest_cache_ . ProvideUpdate(moo_identity, new_manifest, 10);
          auto cb = [ this, moo_identity ](){ this -> DistributeUpdatedManifest(moo_identity); };
          thread_pool_ -> Post( cb );

          it = promised_manifests_ . erase(it);
          FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Success");
        }
        catch(...)
        {
          FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: ERK! whole reading/updating");
          it = promised_manifests_ . erase(it);
          throw;
        }
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: NEW MANIFEST FAILED*****************");
        it = promised_manifests_ . erase(it);
      }
    }
    else
    {
      ++it;
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

void P2PService2::PeerIdentificationSucceeded(const P2PService2::Peer &peer, const P2PService2::Identity &identity)
{
}

void P2PService2::PeerIdentificationFailed   (const P2PService2::Peer &peer)
{
}

void P2PService2::PeerTrustEvent(const Identity &          identity
                                 , P2PTrustFeedbackSubject subject
                                 , P2PTrustFeedbackQuality quality)
{
}

void P2PService2::SetLocalManifest(const Manifest &manifest)
{
  manifest_ = manifest;
  local_services_ . MakeFromManifest(manifest);

  thread_pool_ -> Post([this](){ this -> Refresh(); });
}



} // namespace p2p
} // namespace fetc
