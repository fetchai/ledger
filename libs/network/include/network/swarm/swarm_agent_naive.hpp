#ifndef SWARM_AGENT_NAIVE__
#define SWARM_AGENT_NAIVE__

#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>

#include "network/swarm/swarm_agent_api.hpp"
#include "network/swarm/swarm_random.hpp"

namespace fetch
{
namespace swarm
{

class SwarmAgentNaive
{
public:
  SwarmAgentNaive(SwarmAgentNaive &rhs)            = delete;
  SwarmAgentNaive(SwarmAgentNaive &&rhs)           = delete;
  SwarmAgentNaive operator=(SwarmAgentNaive &rhs)  = delete;
  SwarmAgentNaive operator=(SwarmAgentNaive &&rhs) = delete;

  std::set<std::string> onceAndFuturePeers_;
  std::set<std::string> initialPeers_;

  std::shared_ptr<fetch::swarm::SwarmRandom> rnd_;
  std::string identifier_;
  unsigned int maxpeers_;
  unsigned int blockCounter_;
  int id_;

  virtual ~SwarmAgentNaive()
  {
  }

  void addInitialPeer(const std::string &host)
  {
    initialPeers_.insert(host);
    onceAndFuturePeers_.insert(host);
  }



  SwarmAgentNaive(std::shared_ptr<SwarmAgentApi> api,
                  const std::string &identifier,
                  int id,
                  std::shared_ptr<fetch::swarm::SwarmRandom> rnd,
                  unsigned int maxpeers
                  ):
    rnd_(rnd),
    identifier_(identifier),
    maxpeers_(maxpeers),
    blockCounter_(0),
    id_(id)
  {
    api -> OnIdle([this, api, identifier]{
        auto goodPeers = api -> GetPeers(10, -0.5);
        if (goodPeers.empty())
          {
            return;
          }
        {
          auto host = this -> rnd_ -> pickOneWeighted(goodPeers,
                                                      [api](const std::string &host){
                                                        return api -> GetKarma(host);
                                                      });
          api -> DoPing(host);
          api -> DoDiscoverBlocks(host, 10);
        }
        {
          auto host = this -> rnd_ -> pickOne(goodPeers);
          api -> DoDiscoverBlocks(host, 10);
        }
      });

    api -> OnPeerless([this, api]() {
        for(auto peer : this->initialPeers_)
          {
            if (api -> queryOwnLocation() != peer)
              {
                api -> DoPing(peer);
              }
          }
        for(auto peer : this->onceAndFuturePeers_)
          {
            if (api -> queryOwnLocation() != peer)
              {
                api -> DoPing(peer);
              }
          }
      });

    api -> OnNewPeerDiscovered([this, api, identifier](const std::string &host) {
        if (api -> queryOwnLocation() != host)
          if (std::find(this -> onceAndFuturePeers_.begin(), this -> onceAndFuturePeers_.end(), host)
              == this -> onceAndFuturePeers_.end())
            {
              this -> onceAndFuturePeers_.insert(host);
              api -> DoPing(host);
            }
      });

    api -> OnPingSucceeded([this, identifier, api](const std::string &host) {
        if (api -> queryOwnLocation() != host)
          {
            this -> onceAndFuturePeers_.insert(host);
            api -> AddKarmaMax(host, 1.0, 3.0);
          }
      });

    api -> OnPingFailed([identifier, api](const std::string &host) {
        api -> AddKarma(host, -5.0);
      });

    api -> OnNewBlockIdFound([api, identifier](const std::string &host, const std::string &blockid) {
        api -> AddKarmaMax(host, 1.0, 6.0);
        api -> DoGetBlock(host, blockid);
      });

    api -> OnBlockIdRepeated([](const std::string &host, const std::string &blockid) {
      });

    api -> OnNewBlockAvailable([api, identifier](const std::string &host, const std::string &blockid) {
        api-> VerifyBlock(blockid, true);
        api -> AddKarmaMax(host, 2.0, 10.0);
      });
  }
};

}
}

#endif //__SWARM_AGENT_NAIVE__
