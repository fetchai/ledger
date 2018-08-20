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

#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "network/swarm/swarm_agent_api.hpp"
#include "network/swarm/swarm_random.hpp"

namespace fetch {
namespace swarm {

class SwarmAgentNaive
{
public:
  SwarmAgentNaive(SwarmAgentNaive &rhs)  = delete;
  SwarmAgentNaive(SwarmAgentNaive &&rhs) = delete;
  SwarmAgentNaive operator=(SwarmAgentNaive &rhs) = delete;
  SwarmAgentNaive operator=(SwarmAgentNaive &&rhs) = delete;

  virtual ~SwarmAgentNaive() {}

  void addInitialPeer(const std::string &host)
  {
    initial_peers_.insert(host);
    once_and_future_peers_.insert(host);
  }

  SwarmAgentNaive(std::shared_ptr<SwarmAgentApi> api, const std::string &identifier, int id,
                  std::shared_ptr<fetch::swarm::SwarmRandom> rnd)
    : rnd_(std::move(rnd))
  {
    api->OnIdle([this, api, identifier] {
      auto goodPeers = api->GetPeers(10, -0.5);
      if (goodPeers.empty())
      {
        return;
      }
      {
        auto host = this->rnd_->pickOneWeighted(
            goodPeers, [api](const std::string &host) { return api->GetKarma(host); });
        api->DoPing(host);
        api->DoDiscoverBlocks(host, 10);
      }
      {
        auto host = this->rnd_->pickOne(goodPeers);
        api->DoDiscoverBlocks(host, 10);
      }
    });

    api->OnPeerless([this, api]() {
      for (auto peer : this->initial_peers_)
      {
        if (api->queryOwnLocation() != peer)
        {
          api->DoPing(peer);
        }
      }
      for (auto peer : this->once_and_future_peers_)
      {
        if (api->queryOwnLocation() != peer)
        {
          api->DoPing(peer);
        }
      }
    });

    api->OnNewPeerDiscovered([this, api, identifier](const std::string &host) {
      if (api->queryOwnLocation() != host)
        if (std::find(this->once_and_future_peers_.begin(), this->once_and_future_peers_.end(),
                      host) == this->once_and_future_peers_.end())
        {
          this->once_and_future_peers_.insert(host);
          api->DoPing(host);
        }
    });

    api->OnPingSucceeded([this, identifier, api](const std::string &host) {
      if (api->queryOwnLocation() != host)
      {
        this->once_and_future_peers_.insert(host);
        api->AddKarmaMax(host, 1.0, 3.0);
      }
    });

    api->OnPingFailed([identifier, api](const std::string &host) { api->AddKarma(host, -5.0); });

    api->OnNewBlockIdFound([api, identifier](const std::string &host, const std::string &blockid) {
      api->AddKarmaMax(host, 1.0, 6.0);
      api->DoGetBlock(host, blockid);
    });

    api->OnBlockIdRepeated([](const std::string &host, const std::string &blockid) {});

    api->OnNewBlockAvailable(
        [api, identifier](const std::string &host, const std::string &blockid) {
          api->VerifyBlock(blockid, true);
          api->AddKarmaMax(host, 2.0, 10.0);
        });
  }

private:
  std::shared_ptr<fetch::swarm::SwarmRandom> rnd_;
  std::set<std::string>                      initial_peers_;
  std::set<std::string>                      once_and_future_peers_;
};

}  // namespace swarm
}  // namespace fetch
