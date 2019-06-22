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
#include "ledger/consensus/stake_update_interface.hpp"

#include <unordered_map>
#include <map>

namespace fetch {
namespace ledger {

/**
 * Holds a queue of stake updates that need to be applied at a block interval in the future
 */
class StakeUpdateQueue : public StakeUpdateInterface
{
public:

  // Construction / Destruction
  StakeUpdateQueue() = default;
  StakeUpdateQueue(StakeUpdateQueue const &) = delete;
  StakeUpdateQueue(StakeUpdateQueue &&) = delete;
  ~StakeUpdateQueue() override = default;

  /// @name Basic Interaction
  /// @{
  void AddStakeUpdate(BlockIndex block_index, Address const &address, StakeAmount stake) override;
  void ApplyUpdates(BlockIndex block_index, StakeSnapshot &tracker) override;
  /// @}

  /// @name Accessors
  /// @{
  template <typename Visitor>
  void VisitUnderlyingQueue(Visitor &&visitor);

  std::size_t size() const;
  /// @}

  // Operators
  StakeUpdateQueue &operator=(StakeUpdateQueue const &) = delete;
  StakeUpdateQueue &operator=(StakeUpdateQueue &&) = delete;

private:
  using StakeMap     = std::unordered_map<Address, StakeAmount>;
  using BlockUpdates = std::map<BlockIndex, StakeMap>;

  BlockUpdates updates_{};  ///< The update queue
};

/**
 * Gets the number of block updates currently pending in the queue

 * @return The number of pending updates
 */
inline std::size_t StakeUpdateQueue::size() const
{
  return updates_.size();
}

/**
 * Adds / Updates the change of stake to be applied at a given block index
 *
 * @param block_index The block index at which to apply the update
 * @param address The address of the stake holder
 * @param stake The amount of tokens to be staked
 */
inline void StakeUpdateQueue::AddStakeUpdate(BlockIndex block_index, Address const &address,
                                             StakeAmount stake)
{
  updates_[block_index][address] = stake;
}

/**
 * Visit the underlying queue container directly
 *
 * @tparam Visitor The type of the visitor functor
 * @param visitor The visiting functor
 */
template <typename Visitor>
void StakeUpdateQueue::VisitUnderlyingQueue(Visitor &&visitor)
{
  visitor(updates_);
}

} // namespace ledger
} // namespace fetch
