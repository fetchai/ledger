#ifndef SWARM_AGENT_NAIVE__
#define SWARM_AGENT_NAIVE__

#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <memory>

#include "network/swarm/swarm_agent_api.hpp"
#include "network/swarm/swarm_random.hpp"
#include "swarm_parcel.hpp"

namespace fetch
{
namespace swarm
{

  using std::cout;
  using std::cerr;
  using std::cerr;

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
                  unsigned int maxpeers,
                  unsigned int solvespeed
                  ):
    rnd_(rnd),
    identifier_(identifier),
    maxpeers_(maxpeers),
    blockCounter_(0),
    id_(id)
  {
    api -> OnIdle([this, api, id, identifier, solvespeed]{
        cout << "AGENT NAIVE: OnIdle"<<std::endl;

        if (this -> rnd_ -> r(solvespeed) <= static_cast<unsigned int>(id)) 
          {
            std::ostringstream ret;
            ret << "\"";
            ret << "block by " << identifier_ << " number " << std::to_string(blockCounter_);
            this -> blockCounter_ += 1;
            ret << "\"";

            SwarmParcel newParcel("block", ret.str());

            std::cout << identifier << " solved " << newParcel.GetName() << std::endl;
            api -> DoBlockSolved(ret.str());
          }

        auto goodPeers = api -> GetPeers(solvespeed, -0.5);
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
              std::cout << identifier << " discovered " << host << std::endl;

              this -> onceAndFuturePeers_.insert(host);
              api -> DoPing(host);
            }
      });

    api -> OnPingSucceeded([this, identifier, api](const std::string &host) {
        std::cout << identifier << " confirmed " << host << std::endl;
        if (api -> queryOwnLocation() != host)
          {
            this -> onceAndFuturePeers_.insert(host);
            api -> AddKarmaMax(host, 1.0, 3.0);
          }
      });

    api -> OnPingFailed([identifier, api](const std::string &host) {
        std::cout << identifier << " lost " << host << std::endl;
        api -> AddKarma(host, -5.0);
      });

    api -> OnNewBlockIdFound([api, identifier](const std::string &host, const std::string &blockid) {
        std::cout << identifier << " determined " << host << " has " << blockid << std::endl;
        api -> AddKarmaMax(host, 1.0, 6.0);
        api -> DoGetBlock(host, blockid);
      });

    api -> OnBlockIdRepeated([](const std::string &host, const std::string &blockid) {
      });

    api -> OnNewBlockAvailable([api, identifier](const std::string &host, const std::string &blockid) {
        std::cout << identifier << " obtained " << blockid << " from " << host << std::endl;
        api-> VerifyBlock(blockid, true);
        api -> AddKarmaMax(host, 2.0, 10.0);
      });
  }
};

}
}

#endif //__SWARM_AGENT_NAIVE__
