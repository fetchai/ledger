#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "core/threading/synchronised_state.hpp"
#include "muddle/address.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "network/service/protocol.hpp"
#include "network/peer.hpp"

#include <vector>

namespace fetch {
namespace muddle {

class Router;

class DiscoveryService : public service::Protocol
{
public:
  enum
  {
    CONNECTION_INFORMATION = 1,
  };

  using Peers = std::vector<network::Peer>;

  // Construction / Destruction
  DiscoveryService();
  DiscoveryService(DiscoveryService const &) = delete;
  DiscoveryService(DiscoveryService &&)      = delete;
  ~DiscoveryService() override               = default;

  void UpdatePeers(Peers peers);

  // Operators
  DiscoveryService &operator=(DiscoveryService const &) = delete;
  DiscoveryService &operator=(DiscoveryService &&) = delete;

private:
  using SubscriptionPtr = MuddleEndpoint::SubscriptionPtr;
  using SyncPeers = SynchronisedState<Peers>;

  Peers GetConnectionInformation();

  SyncPeers possible_peers_{};
};

}  // namespace muddle
}  // namespace fetch
