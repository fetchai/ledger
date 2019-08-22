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
 * The stake manager manages and verifies who the stakers are on a block by block basis (stake snapshot). This is a separate
 * class to the wallet record and so does not necessarily get written to the state database.
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
  StakeManager(uint64_t committee_size, uint32_t block_interval_ms = 1000, uint64_t snapshot_validity_periodicity = 1);
  StakeManager(StakeManager const &) = delete;
  StakeManager(StakeManager &&)      = delete;
  ~StakeManager() override           = default;

  /// @name Stake Manager Interface
  /// @{
  void        UpdateCurrentBlock(Block const &current) override;
  uint64_t    GetBlockGenerationWeight(Block const &previous, Address const &address) override;
  bool        ShouldGenerateBlock(Block const &previous, Address const &address) override;
  bool        ValidMinerForBlock(Block const &previous, Address const &address) override;
  /// @}

  uint32_t BlockInterval();

  // TODO(HUT): promote this to the interface (?)
  CommitteePtr GetCommittee(Block const &previous);

  // Accessors for the executor
  StakeUpdateQueue &      update_queue();
  StakeUpdateQueue const &update_queue() const;
  uint64_t               committee_size() const;

  std::shared_ptr<StakeSnapshot const> GetCurrentStakeSnapshot() const;

  void Reset(StakeSnapshot const &snapshot);
  void Reset(StakeSnapshot &&snapshot);

  // Operators
  StakeManager &operator=(StakeManager const &) = delete;
  StakeManager &operator=(StakeManager &&) = delete;

private:
  static constexpr std::size_t HISTORY_LENGTH = 1000;

  using BlockIndex           = uint64_t;
  using StakeSnapshotPtr     = std::shared_ptr<StakeSnapshot>;
  using StakeHistory         = std::map<BlockIndex, StakeSnapshotPtr>;
  using CommitteeHistory     = std::map<BlockIndex, CommitteePtr>;
  /* using EntropyCache     = std::map<BlockIndex, uint64_t>; */

  StakeSnapshotPtr LookupStakeSnapshot(BlockIndex block);
  void             ResetInternal(StakeSnapshotPtr &&snapshot);
  /* bool             LookupEntropy(Block const &block, uint64_t &entropy); */

  // Config & Components
  uint64_t                   committee_size_{0};       ///< The "static" size of the committee
  uint64_t                   snapshot_validity_periodicity_{1}; ///< The period to use when building committees

  /* EntropyGeneratorInterface *entropy_{nullptr};     ///< The reference to entropy module */
  StakeUpdateQueue           update_queue_;            ///< The update queue of events
  StakeHistory               stake_history_{};               ///< Cache of historical snapshots
  CommitteeHistory           committee_history_{};               ///< Cache of historical committees
  StakeSnapshotPtr           current_{};               ///< Most recent snapshot
  BlockIndex                 current_block_index_{0};  ///< Block index of most recent snapshot
  /* EntropyCache               entropy_cache_{}; */
  uint32_t                   block_interval_ms_{std::numeric_limits<uint32_t>::max()};
};

inline uint64_t StakeManager::committee_size() const
{
  return committee_size_;
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

}  // namespace ledger
}  // namespace fetch
