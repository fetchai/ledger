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

#include "address/address.hpp"
#include "ledger/consensus/stake_manager_interface.hpp"
#include "ledger/consensus/stake_update_queue.hpp"

#include <vector>

namespace fetch {

namespace crypto {
class Identity;
}

namespace ledger {

class StakeSnapshot;
class EntropyGeneratorInterface;

/**
 * The stake manager manages and verifies who the stakers are on a block by block basis (stake
 * snapshot). This is a separate class to the wallet record and so does not necessarily get written
 * to the state database.
 *
 * During normal operation, transactions that execute staking or destaking events will be
 * collected after block execution and sent to the StakeManager. These go into a queue
 * aimed at enforcing a cool down and spin-up period for stakers.
 *
 * Blocks and stake updates passed to the stake manager are assumed to be valid, including
 * the entropy within the block. The entropy together with the maximum stakers allowed
 * can be used to deterministically build a committee.
 *
 */
class StakeManager final : public StakeManagerInterface
{
public:
  using Identity     = crypto::Identity;
  using Committee    = std::vector<Identity>;
  using CommitteePtr = std::shared_ptr<Committee const>;

  // Construction / Destruction
  explicit StakeManager(uint64_t committee_size);
  StakeManager(StakeManager const &) = delete;
  StakeManager(StakeManager &&)      = delete;
  ~StakeManager() override           = default;

  /// @name Stake Manager Interface
  /// @{
  void         UpdateCurrentBlock(Block const &current) override;
  CommitteePtr BuildCommittee(Block const &current);
  /// @}

  uint32_t BlockInterval();

  // Accessors for the executor
  StakeUpdateQueue &      update_queue();
  StakeUpdateQueue const &update_queue() const;
  uint64_t                committee_size() const;
  void                    SetCommitteeSize(uint64_t size);

  std::shared_ptr<StakeSnapshot const> GetCurrentStakeSnapshot() const;

  StakeManager::CommitteePtr Reset(StakeSnapshot const &snapshot);
  StakeManager::CommitteePtr Reset(StakeSnapshot &&snapshot);

  // Operators
  StakeManager &operator=(StakeManager const &) = delete;
  StakeManager &operator=(StakeManager &&) = delete;

private:
  static constexpr std::size_t HISTORY_LENGTH = 1000;

  using BlockIndex       = uint64_t;
  using StakeSnapshotPtr = std::shared_ptr<StakeSnapshot>;
  using StakeHistory     = std::map<BlockIndex, StakeSnapshotPtr>;

  StakeSnapshotPtr           LookupStakeSnapshot(BlockIndex block);
  StakeManager::CommitteePtr ResetInternal(StakeSnapshotPtr &&snapshot);

  // Config & Components
  uint64_t committee_size_{0};  ///< The "static" size of the committee

  StakeUpdateQueue update_queue_;            ///< The update queue of events
  StakeHistory     stake_history_{};         ///< Cache of historical snapshots
  StakeSnapshotPtr current_{};               ///< Most recent snapshot
  BlockIndex       current_block_index_{0};  ///< Block index of most recent snapshot
};

inline uint64_t StakeManager::committee_size() const
{
  return committee_size_;
}

inline void StakeManager::SetCommitteeSize(uint64_t size)
{
  committee_size_ = size;
}

inline StakeUpdateQueue &StakeManager::update_queue()
{
  return update_queue_;
}

inline StakeUpdateQueue const &StakeManager::update_queue() const
{
  return update_queue_;
}

inline std::shared_ptr<StakeSnapshot const> StakeManager::GetCurrentStakeSnapshot() const
{
  return current_;
}

template <typename T>
void TrimToSize(T &container, uint64_t max_allowed)
{
  if (container.size() >= max_allowed)
  {
    auto const num_to_remove = container.size() - max_allowed;

    if (num_to_remove > 0)
    {
      auto end = container.begin();
      std::advance(end, static_cast<std::ptrdiff_t>(num_to_remove));

      container.erase(container.begin(), end);
    }
  }
}

}  // namespace ledger
}  // namespace fetch
