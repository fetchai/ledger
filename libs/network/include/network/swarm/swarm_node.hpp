#ifndef SWARM_NODE__
#define SWARM_NODE__

#include "swarm_peer_location.hpp"
#include "swarm_karma_peers.hpp"
#include "swarm_random.hpp"
#include "exception.hpp"
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include"network/service/client.hpp"
#include"network/service/server.hpp"
#include"http/script/variant.hpp"
#include"http/json/document.hpp"
#include "network/protocols/swarm/commands.hpp"
#include "network/protocols/fetch_protocols.hpp"

#include <iostream>
#include <string>

namespace fetch
{
namespace swarm
{

  using std::cout;
  using std::cerr;
  using std::cerr;

class SwarmHttpInterface;

class SwarmNode
{
public:
  using clientType = fetch::service::ServiceClient<network::TCPClient>;
protected:
  typedef std::recursive_mutex MUTEX_T;
  typedef std::lock_guard<MUTEX_T> LOCK_T;
  typedef std::map<std::string, std::shared_ptr<clientType>> CONNECTIONS;
public:
  explicit SwarmNode(
                     fetch::network::ThreadManager tm,
                     const std::string &identifier,
                     unsigned int maxpeers,
                     std::shared_ptr<fetch::swarm::SwarmRandom> rnd,
                     const fetch::swarm::SwarmPeerLocation &uri,
                     unsigned int protocolNumber
                     ):
    uri_(uri),
    rnd_(rnd),
    tm_(tm),
    karmaPeerList_(identifier),
    protocolNumber_(protocolNumber)
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

  virtual std::shared_ptr<clientType> ConnectToPeer(const SwarmPeerLocation &peer)
  {
    LOCK_T mlock(mutex_);
    std::shared_ptr<clientType> client = std::make_shared<clientType>( peer.GetHost(), peer.GetPort(), tm_ );

    int waits = 25;
    while(!client->is_alive())
      {
        waits--;
        if (waits <= 0)
          {
            throw SwarmException("Bad peer "+peer.AsString());
          }
        usleep(1000);
      }
    return client;
  }

  std::list<SwarmKarmaPeer> HttpWantsPeerList() const
  {
    return karmaPeerList_.GetBestPeers(10000, 0.0);
  }

  void ToGetState(std::function<int()> cb) { toGetState_ = cb; }

  virtual std::string AskPeerForPeers(const SwarmPeerLocation &peer)
  {
    std::shared_ptr<clientType> client = ConnectToPeer(peer);
    auto promise = client->Call(protocolNumber_, protocols::Swarm::CLIENT_NEEDS_PEER);
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
    tm_ . Post(workload);
  }

protected:
  mutable MUTEX_T                            mutex_;
  int                                        maxActivePeers_;
  int                                        maxKnownPeers_;
  std::string                                identifier_;
  unsigned int                               maxpeers_;
  SwarmPeerLocation                          uri_;
  std::shared_ptr<fetch::swarm::SwarmRandom> rnd_;
  fetch::network::ThreadManager              tm_;
  CONNECTIONS                                connections;
  SwarmKarmaPeers                            karmaPeerList_;
  unsigned int                               protocolNumber_;
  std::function<int()>                       toGetState_;
};

}
}

#endif //__SWARM_NODE__
