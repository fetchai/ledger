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

#include <iostream>
#include <string>

#include "network/protocols/fetch_protocols.hpp"

namespace fetch {
namespace swarm {

class SwarmProtocol;

class SwarmNodeInterface
{
public:
  static const uint32_t protocol_number = fetch::protocols::FetchProtocols::SWARM;
  using protocol_class_type             = SwarmProtocol;

  SwarmNodeInterface() {}

  virtual ~SwarmNodeInterface() {}

  virtual std::string ClientNeedsPeer() = 0;
};

}  // namespace swarm
}  // namespace fetch
