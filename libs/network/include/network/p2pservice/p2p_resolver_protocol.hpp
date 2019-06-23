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

#include "network/service/protocol.hpp"

namespace fetch {
namespace p2p {

class P2PService;
class Resolver;

/**
 * Protocol for the P2P address resolution protocol
 */
class ResolverProtocol : public service::Protocol
{
public:
  enum
  {
    QUERY                 = 1,
    GET_MANIFEST          = 2,
    GET_RANDOM_GOOD_PEERS = 3,
    GET_NODE_URI          = 4
  };

  explicit ResolverProtocol(Resolver &resolver, P2PService &p2p_service);
};

}  // namespace p2p
}  // namespace fetch
