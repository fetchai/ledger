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

#include "network/p2pservice/p2p_resolver_protocol.hpp"

#include "network/p2pservice/p2p_resolver.hpp"
#include "network/p2pservice/p2p_service.hpp"

namespace fetch {
namespace p2p {

ResolverProtocol::ResolverProtocol(Resolver &resolver, P2PService &p2p_service)
{
  Expose(QUERY, &resolver, &Resolver::Query);
  Expose(GET_MANIFEST, &p2p_service, &P2PService::GetLocalManifest);
  Expose(GET_RANDOM_GOOD_PEERS, &p2p_service, &P2PService::GetRandomGoodPeers);
  Expose(GET_NODE_URI, &p2p_service, &P2PService::GetNodeUri);
}

}  // namespace p2p
}  // namespace fetch
