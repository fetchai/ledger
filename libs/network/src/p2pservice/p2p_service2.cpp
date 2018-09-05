#include "network/p2pservice/p2p_service2.hpp"
#include "network/p2pservice/p2ptrust.hpp"

namespace fetch {
namespace p2p {

P2PService2::P2PService2(muddle_service_type muddle)
  : muddle_(muddle)
  , resolver_proto_{resolver_, *this}
{
  // register the services with the rpc server
  rpc_server_.Add(PROTOCOL_RESOLVER, &resolver_proto_);
  trust_system = std::make_shared<P2PTrust<Identity>>();
}

void P2PService2::Start(P2PService2::PeerList const &initial_peer_list)
{
  for(auto &peer : initial_peer_list)
  {
    possibles_.push_back(peer . ToUri());
  }

  thread_pool_ -> SetInterval(1000);
  thread_pool_ -> Start();
  thread_pool_ -> PostIdle([this](){ this -> WorkCycle(); });
}

void P2PService2::Stop()
{
  thread_pool_ -> clear();
  thread_pool_ -> Stop();
}

void P2PService2::WorkCycle()
{
  FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle");
  // see how many peers we have.

  using first_t  = typename std::tuple_element<0, Muddle::ConnectionData>::type;
  using second_t = typename std::tuple_element<1, Muddle::ConnectionData>::type;
  using third_t  = typename std::tuple_element<2, Muddle::ConnectionData>::type;

  std::set<Uri> used;

  auto connections = muddle_ . GetConnections(); // address/uri/state tuples.
  FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Conncount = ", connections.size());
  for(auto connection : connections)
  {

    auto addr  = std::get<0>(connection);
    network::Uri uri   = std::get<1>(connection);
    auto state = std::get<2>(connection);

    FETCH_LOG_WARN(LOGGING_NAME,"P2PService2::WorkCycle: Conn:", ToHex(addr), " / ", uri.ToString(), " / ", state);
    used.insert(uri);
  }

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

  // too many? schedule some kickoffs.
}

const network::Manifest &P2PService2::GetLocalManifest()
{
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

  for(auto &manifest_entry : manifest_)
  {
    auto service_id = manifest_entry.first;
    auto uri = manifest_entry.second;

    if (local_services_.find(service_id) != local_services_.end())
    {
      continue;
    }

    local_services_[service_id] = std::make_shared<P2PManagedLocalService>(uri, service_id);
  }

  for(auto local_service_entry : local_services_)
  {
    auto service_id = local_service_entry.first;
    auto uri = local_service_entry.second;

    if (!manifest_.ContainsService(service_id))
    {
      local_services_.erase(service_id);
    }

  }
}



} // namespace p2p
} // namespace fetch

