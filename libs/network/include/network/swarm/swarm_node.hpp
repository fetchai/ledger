#ifndef SWARM_NODE__
#define SWARM_NODE__

#include <string>
#include <unistd.h>
#include <cstdlib>

#include "swarm_peer_location.hpp"
#include "swarm_karma_peers.hpp"
#include "swarm_random.hpp"
#include "exception.hpp"
#include "core/script/variant.hpp"
#include "core/json/document.hpp"
#include "network/service/client.hpp"
#include "network/service/server.hpp"
#include "network/protocols/swarm/commands.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/interfaces/swarm/swarm_node_interface.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "network/protocols/swarm/swarm_protocol.hpp"

#include <iostream>
#include <string>

namespace fetch
{
namespace swarm
{

class SwarmHttpInterface;

class SwarmNode : public SwarmNodeInterface
{
public:
protected:
  typedef std::recursive_mutex mutex_type;
  typedef std::lock_guard<mutex_type> lock_type;
  typedef fetch::network::NetworkNodeCore::client_type client_type;
public:

  std::shared_ptr<fetch::network::NetworkNodeCore> nnCore_;

  explicit SwarmNode(
                     std::shared_ptr<fetch::network::NetworkNodeCore> networkNodeCore,
                     const std::string &identifier,
                     unsigned int maxpeers,
                     std::shared_ptr<fetch::swarm::SwarmRandom> rnd,
                     const fetch::swarm::SwarmPeerLocation &uri
                     ):
    nnCore_(networkNodeCore),
    uri_(uri),
    rnd_(rnd),
    karmaPeerList_(identifier)
  {
    identifier_ = identifier;
    maxpeers_ = maxpeers;

    nnCore_ -> AddProtocol(this);
  }

  explicit SwarmNode(
                     const std::string &identifier,
                     unsigned int maxpeers,
                     std::shared_ptr<fetch::swarm::SwarmRandom> rnd,
                     const fetch::swarm::SwarmPeerLocation &uri
                     ):
    uri_(uri),
    rnd_(rnd),
    karmaPeerList_(identifier)
  {
    identifier_ = identifier;
    maxpeers_ = maxpeers;
  }

  virtual ~SwarmNode()
  {
  }

  SwarmNode(SwarmNode &rhs)            = delete;
  SwarmNode(SwarmNode &&rhs)           = delete;
  SwarmNode operator=(SwarmNode &rhs)  = delete;
  SwarmNode operator=(SwarmNode &&rhs) = delete;

  virtual SwarmPeerLocation GetPingablePeer()
  {
    return karmaPeerList_.GetNthKarmicPeer(maxpeers_).GetLocation();
  }

  virtual bool HasPeers()
  {
    return !karmaPeerList_.empty();
  }

  virtual bool IsOwnLocation(const SwarmPeerLocation &loc) const
  {
    return loc == uri_;
  }

  virtual std::shared_ptr<client_type> ConnectToPeer(const SwarmPeerLocation &peer)
  {
    return nnCore_ -> ConnectTo(peer.GetHost(), peer.GetPort());
  }

  std::list<SwarmKarmaPeer> HttpWantsPeerList() const
  {
    return karmaPeerList_.GetBestPeers(10000, 0.0);
  }

   void ToGetState(std::function<int()> cb) { toGetState_ = cb; }

  virtual std::string AskPeerForPeers(const SwarmPeerLocation &peer)
  {
    std::shared_ptr<client_type> client = ConnectToPeer(peer);
    auto promise = client->Call(protocol_number, protocols::Swarm::CLIENT_NEEDS_PEER);
    promise.Wait();
    auto result = promise.As<std::string>();
    return result;
  }

  virtual int GetState()
  {
    if (toGetState_)
      {
        return toGetState_();
      }
    else
      {
        return 0;
      }
  }

  virtual std::string ClientNeedsPeer()
  {
    if (!karmaPeerList_.empty())
      {
        auto p = karmaPeerList_.GetNthKarmicPeer(0);
        auto s = p.GetLocation().AsString();
        return s;
      }
    return std::string("");
  }

  const std::string &GetId()
  {
    return identifier_;
  }

  void AddOrUpdate(const std::string &host, double karma)
  {
    karmaPeerList_.AddOrUpdate(host, karma);
  }
  void AddOrUpdate(const SwarmPeerLocation &host, double karma)
  {
    karmaPeerList_.AddOrUpdate(host, karma);
  }
  double GetKarma(const std::string &host)
  {
    return karmaPeerList_.GetKarma(host);
  }

  std::list<SwarmKarmaPeer> GetBestPeers(unsigned int n, double minKarma = 0.0) const
  {
    return karmaPeerList_.GetBestPeers(n, minKarma);
  }

  void Post(std::function<void ()> workload)
  {
    nnCore_ -> Post(workload, 0);
  }

protected:
  mutable mutex_type                         mutex_;
  int                                        maxActivePeers_;
  int                                        maxKnownPeers_;
  std::string                                identifier_;
  unsigned int                               maxpeers_;
  SwarmPeerLocation                          uri_;
  std::shared_ptr<fetch::swarm::SwarmRandom> rnd_;
  SwarmKarmaPeers                            karmaPeerList_;
  unsigned int                               protocolNumber_;
  std::function<int()>                       toGetState_;
};

}
}

#endif //__SWARM_NODE__
