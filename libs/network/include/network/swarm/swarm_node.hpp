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

#include <cstdlib>
#include <string>
#include <unistd.h>

#include "core/json/document.hpp"
#include "core/script/variant.hpp"
#include "exception.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/interfaces/swarm/swarm_node_interface.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "network/protocols/swarm/commands.hpp"
#include "network/protocols/swarm/swarm_protocol.hpp"
#include "network/service/client.hpp"
#include "network/service/server.hpp"
#include "swarm_karma_peers.hpp"
#include "swarm_peer_location.hpp"
#include "swarm_random.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace fetch {
namespace swarm {

class SwarmHttpInterface;

class SwarmNode : public SwarmNodeInterface
{
public:
  using clientType = fetch::service::ServiceClient;

protected:
  using mutex_type  = std::recursive_mutex;
  using lock_type   = std::lock_guard<mutex_type>;
  using client_type = fetch::network::NetworkNodeCore::client_type;

public:

  static constexpr char const *LOGGING_NAME = "SwarmNode";

  explicit SwarmNode(std::shared_ptr<fetch::network::NetworkNodeCore> networkNodeCore,
                     const std::string &identifier, uint32_t maxpeers,
                     fetch::swarm::SwarmPeerLocation uri)
    : nn_core_(std::move(networkNodeCore)), uri_(std::move(uri)), karmaPeerList_(identifier)
  {
    identifier_ = identifier;
    maxpeers_   = maxpeers;

    nn_core_->AddProtocol(this);
  }

  explicit SwarmNode(fetch::network::NetworkManager tm, const std::string &identifier,
                     uint32_t maxpeers, fetch::swarm::SwarmPeerLocation uri)
    : uri_(std::move(uri)), karmaPeerList_(identifier)
  {
    identifier_ = identifier;
    maxpeers_   = maxpeers;
  }

  virtual ~SwarmNode() {}

  SwarmNode(SwarmNode &rhs)  = delete;
  SwarmNode(SwarmNode &&rhs) = delete;
  SwarmNode operator=(SwarmNode &rhs) = delete;
  SwarmNode operator=(SwarmNode &&rhs) = delete;

  virtual SwarmPeerLocation GetPingablePeer()
  {
    return karmaPeerList_.GetNthKarmicPeer(maxpeers_).GetLocation();
  }

  virtual bool HasPeers() { return !karmaPeerList_.empty(); }

  virtual bool IsOwnLocation(const SwarmPeerLocation &loc) const { return loc == uri_; }

  std::list<SwarmKarmaPeer> HttpWantsPeerList() const
  {
    return karmaPeerList_.GetBestPeers(10000, 0.0);
  }

  void ToGetState(std::function<int()> cb) { toGetState_ = cb; }

  virtual std::string AskPeerForPeers(const SwarmPeerLocation &    peer,
                                      std::shared_ptr<client_type> client)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME,"AskPeerForPeers starts work");

    auto promise = client->Call(protocol_number, protocols::Swarm::CLIENT_NEEDS_PEER);
    FETCH_LOG_PROMISE();
    if (promise.Wait(2500, false))
    {
      auto result = promise.As<std::string>();
      return result;
    }
    else
    {
      if (promise.has_failed())
      {
        FETCH_LOG_DEBUG(LOGGING_NAME,"AskPeerForPeers has_failed");
      }
      else if (promise.is_connection_closed())
      {
        FETCH_LOG_DEBUG(LOGGING_NAME,"AskPeerForPeers is_connection_closed");
      }
      else
      {
        FETCH_LOG_DEBUG(LOGGING_NAME,"AskPeerForPeers failed ???");
      }
      return "";
    }
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

  virtual bool IsExistingPeer(const std::string &host) { return karmaPeerList_.Has(host); }

  virtual std::string ClientNeedsPeer()
  {
    FETCH_LOG_DEBUG(LOGGING_NAME,"ClientNeedsPeer starts work");
    if (!karmaPeerList_.empty())
    {
      auto p = karmaPeerList_.GetNthKarmicPeer(0);
      FETCH_LOG_DEBUG(LOGGING_NAME,"ClientNeedsPeer sorted & found");
      auto s = p.GetLocation().AsString();
      return s;
    }
    FETCH_LOG_DEBUG(LOGGING_NAME,"ClientNeedsPeer no peers");
    return std::string("");
  }

  const std::string &GetId() { return identifier_; }

  void AddOrUpdate(const std::string &host, double karma)
  {
    karmaPeerList_.AddOrUpdate(host, karma);
  }
  void AddOrUpdate(const SwarmPeerLocation &host, double karma)
  {
    karmaPeerList_.AddOrUpdate(host, karma);
  }
  double GetKarma(const std::string &host) { return karmaPeerList_.GetKarma(host); }

  std::list<SwarmKarmaPeer> GetBestPeers(uint32_t n, double minKarma = 0.0) const
  {
    return karmaPeerList_.GetBestPeers(n, minKarma);
  }

  void Post(std::function<void()> workload) { nn_core_->Post(workload); }

protected:
  std::shared_ptr<fetch::network::NetworkNodeCore> nn_core_;
  mutable mutex_type                               mutex_;
  int                                              maxActivePeers_;
  int                                              maxKnownPeers_;
  std::string                                      identifier_;
  uint32_t                                         maxpeers_;
  SwarmPeerLocation                                uri_;
  fetch::network::NetworkManager                   tm_;
  SwarmKarmaPeers                                  karmaPeerList_;
  uint32_t                                         protocolNumber_;
  std::function<int()>                             toGetState_;
};

}  // namespace swarm
}  // namespace fetch
