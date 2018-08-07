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
                     uint32_t maxpeers,
                     const fetch::swarm::SwarmPeerLocation &uri
                     ):
      nnCore_(std::move(networkNodeCore)),
    uri_(uri),
    karmaPeerList_(identifier)
  {
    identifier_ = identifier;
    maxpeers_ = maxpeers;

    nnCore_ -> AddProtocol(this);
  }

  explicit SwarmNode(
                     fetch::network::NetworkManager tm,
                     const std::string &identifier,
                     uint32_t maxpeers,
                     const fetch::swarm::SwarmPeerLocation &uri
                     ):
    uri_(uri),
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
      lock_type lock(mutex_);
    return karmaPeerList_.GetNthKarmicPeer(maxpeers_).GetLocation();
  }

  virtual bool HasPeers()
  {
      lock_type lock(mutex_);
      return !karmaPeerList_.empty();
  }

  virtual bool IsOwnLocation(const SwarmPeerLocation &loc) const
  {
    return loc == uri_;
  }

  std::list<SwarmKarmaPeer> HttpWantsPeerList() const
  {
      return karmaPeerList_.GetBestPeers(10, 0.0);
  }

  std::string HttpWantsSitrep() const
  {
      return GetSitrep();
  }

    virtual std::string AskPeerForPeers(
        const SwarmPeerLocation &peer, std::shared_ptr<client_type> client)
  {
      lock_type lock(mutex_);
      auto promise = client->Call(
        protocol_number, protocols::Swarm::CLIENT_NEEDS_PEER);
    if (promise.Wait(2500, false))
      {
        auto result = promise.As<std::string>();
        return result;
      }
    else
    {
        if (promise. has_failed())
        {
            fetch::logger.Debug("AskPeerForPeers has_failed");
        }
        else if (promise. is_connection_closed())
        {
            fetch::logger.Debug("AskPeerForPeers is_connection_closed");
        }
        else
        {
            fetch::logger.Debug("AskPeerForPeers failed ???");
        }
        return "";
    }
  }

  virtual bool IsExistingPeer(const std::string &host)
    {
        return karmaPeerList_.Has(host);
  }

    virtual std::string ClientNeedsPeer()
    {
        fetch::logger.Debug("ClientNeedsPeer starts work");
        if (!karmaPeerList_.empty())
        {
            auto p = karmaPeerList_.GetNthKarmicPeer(0);
            fetch::logger.Debug("ClientNeedsPeer sorted & found");
            auto s = p.GetLocation().AsString();
            return s;
        }
        fetch::logger.Debug("ClientNeedsPeer no peers");
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
  void SetKarma(const std::string &host, double karma)
  {
    karmaPeerList_.SetKarma(host, karma);
  }
  double GetKarma(const std::string &host)
  {
    return karmaPeerList_.GetKarma(host);
  }

  std::list<SwarmKarmaPeer> GetBestPeers(uint32_t n, double minKarma = 0.0) const
  {
      fetch::logger.Debug("GetBestPeers running");
      return karmaPeerList_.GetBestPeers(n, minKarma);
  }

  void Post(std::function<void ()> workload)
  {
      nnCore_ -> Post(workload);
  }

    void SetSitrep(const std::string &new_sitrep)
    {
        lock_type lock(sitrep_mutex_);
        sitrep_ = new_sitrep;
    }

    std::string GetSitrep(void) const
    {
        lock_type lock(sitrep_mutex_);
        return sitrep_;
    }

protected:
    mutable mutex_type                         mutex_;
    mutable mutex_type                         sitrep_mutex_;
    int                                        maxActivePeers_;
    int                                        maxKnownPeers_;
    std::string                                identifier_;
    uint32_t                                   maxpeers_;
    SwarmPeerLocation                          uri_;
    fetch::network::NetworkManager              tm_;
    SwarmKarmaPeers                            karmaPeerList_;
    uint32_t                                   protocolNumber_;
    std::string                                sitrep_;
};

}
}

#endif //__SWARM_NODE__
