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
#include <map>
#include <memory>

namespace fetch {
namespace ledger {

class Address;
class Block;

class StakeManagerInterface
{
public:
  // Construction / Destruction
  StakeManagerInterface()          = default;
  virtual ~StakeManagerInterface() = default;

  /// @name Stake Manager Interface
  /// @{
  virtual void        UpdateCurrentBlock(Block const &current)                                = 0;
  virtual std::size_t GetBlockGenerationWeight(Block const &previous, Address const &address) = 0;
  virtual bool        ShouldGenerateBlock(Block const &previous, Address const &address)      = 0;
  /// @}

private:
};

}  // namespace ledger
}  // namespace fetch
