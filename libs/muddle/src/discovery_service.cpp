//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "discovery_service.hpp"

namespace fetch {
namespace muddle {

DiscoveryService::DiscoveryService()
{
  Expose(CONNECTION_INFORMATION, this, &DiscoveryService::GetConnectionInformation);
}

void DiscoveryService::UpdatePeers(Peers peers)
{
  possible_peers_.ApplyVoid([&peers](Peers &p) { p = std::move(peers); });
}

DiscoveryService::Peers DiscoveryService::GetConnectionInformation()
{
  return possible_peers_.Apply([](Peers const &peers) { return peers; });
}

}  // namespace muddle
}  // namespace fetch
