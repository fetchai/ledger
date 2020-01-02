#pragma once
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

#include "muddle/address.hpp"
#include "network/uri.hpp"

#include <cstdint>
#include <unordered_map>

namespace fetch {
namespace shards {

class ShardManagementInterface
{
public:
  using LaneIndex     = uint32_t;
  using MuddleAddress = muddle::Address;
  using AddressMap    = std::unordered_map<MuddleAddress, network::Uri>;

  // Construction / Destruction
  ShardManagementInterface()          = default;
  virtual ~ShardManagementInterface() = default;

  /// @name Lane Management
  /// @{
  virtual void UseThesePeers(LaneIndex lane, AddressMap const &address_map) = 0;
  /// @}
};

}  // namespace shards
}  // namespace fetch
