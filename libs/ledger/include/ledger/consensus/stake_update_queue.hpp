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

#include "ledger/chain/address.hpp"

#include <unordered_map>
#include <map>

namespace fetch {
namespace ledger {

class StakeTracker;

class StakeUpdateQueue
{
public:
  using StakeAmount = uint64_t;
  using BlockIndex  = uint64_t;

  // Construction / Destruction
  StakeUpdateQueue() = default;
  StakeUpdateQueue(StakeUpdateQueue const &) = delete;
  StakeUpdateQueue(StakeUpdateQueue &&) = delete;
  ~StakeUpdateQueue() = default;

  void AddStakeUpdate(BlockIndex block_index, Address const &address, StakeAmount stake);
  void ApplyUpdates(BlockIndex block_index, StakeTracker &tracker);

  std::size_t size() const;

  template <typename Visitor>
  void VisitUnderlyingList(Visitor &&visitor)
  {
    visitor(updates_);
  }

  // Operators
  StakeUpdateQueue &operator=(StakeUpdateQueue const &) = delete;
  StakeUpdateQueue &operator=(StakeUpdateQueue &&) = delete;

private:

  using StakeMap     = std::unordered_map<Address, StakeAmount>;
  using BlockUpdates = std::map<BlockIndex, StakeMap>;

  BlockUpdates updates_{};
};

inline std::size_t StakeUpdateQueue::size() const
{
  return updates_.size();
}

inline void StakeUpdateQueue::AddStakeUpdate(BlockIndex block_index, Address const &address,
                                             StakeAmount stake)
{
  updates_[block_index][address] = stake;
}

} // namespace ledger
} // namespace fetch
