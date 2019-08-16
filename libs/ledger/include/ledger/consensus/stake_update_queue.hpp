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
#include "crypto/identity.hpp"
#include "ledger/consensus/stake_update_interface.hpp"

#include <map>
#include <memory>
#include <unordered_map>

namespace fetch {
namespace ledger {

/**
 * Holds a queue of stake updates that need to be applied at a block interval in the future
 */
class StakeUpdateQueue : public StakeUpdateInterface
{
public:
  using StakeSnapshotPtr = std::shared_ptr<StakeSnapshot>;
  using Identity         = crypto::Identity;

  // Construction / Destruction
  StakeUpdateQueue()                         = default;
  StakeUpdateQueue(StakeUpdateQueue const &) = delete;
  StakeUpdateQueue(StakeUpdateQueue &&)      = delete;
  ~StakeUpdateQueue() override               = default;

  /// @name Stake Update Interface
  /// @{
  void AddStakeUpdate(BlockIndex block_index, crypto::Identity const &identity,
                      StakeAmount stake) override;
  /// @}

  bool ApplyUpdates(BlockIndex block_index, StakeSnapshotPtr const &reference,
                    StakeSnapshotPtr &next);

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
  using StakeMap     = std::unordered_map<Identity, StakeAmount>;
  using BlockUpdates = std::map<BlockIndex, StakeMap>;
  using SyncUpdates  = SynchronisedState<BlockUpdates>;

  SyncUpdates updates_{};  ///< The update queue
};

/**
 * Gets the number of block updates currently pending in the queue

 * @return The number of pending updates
 */
inline std::size_t StakeUpdateQueue::size() const
{
  std::size_t size{0};

  updates_.Apply([&size](BlockUpdates const &updates) { size = updates.size(); });

  return size;
}

/**
 * Adds / Updates the change of stake to be applied at a given block index
 *
 * @param block_index The block index at which to apply the update
 * @param identity The identity of the stake holder
 * @param stake The amount of tokens to be staked
 */
inline void StakeUpdateQueue::AddStakeUpdate(BlockIndex block_index, Identity const &identity,
                                             StakeAmount stake)
{
  updates_.Apply([&](BlockUpdates &updates) { updates[block_index][identity] = stake; });
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
  updates_.Apply([&](BlockUpdates &updates) { visitor(updates); });
}

}  // namespace ledger
}  // namespace fetch
