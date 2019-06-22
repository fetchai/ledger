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

#include <cstdint>

namespace fetch {
namespace ledger {

class Address;
class StakeSnapshot;

class StakeUpdateInterface
{
public:
  using StakeAmount = uint64_t;
  using BlockIndex  = uint64_t;

  // Construction / Destruction
  StakeUpdateInterface() = default;
  virtual ~StakeUpdateInterface() = default;

  /// @name Stake Update Interface
  /// @{
  virtual void AddStakeUpdate(BlockIndex block_index, Address const &address, StakeAmount stake) = 0;
  /// @}

};

} // namespace ledger
} // namespace fetch
