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
namespace ledger {

class StakeSnapshot;
class EntropyGeneratorInterface;

class StakeManager final : public StakeManagerInterface
{
public:
  using Committee    = std::vector<Address>;
  using CommitteePtr = std::shared_ptr<Committee const>;

  // Construction / Destruction
  explicit StakeManager(EntropyGeneratorInterface &entropy);
  StakeManager(StakeManager const &) = delete;
  StakeManager(StakeManager &&)      = delete;
  ~StakeManager() override           = default;

  /// @name Stake Manager Interface
  /// @{
  void        UpdateCurrentBlock(Block const &current) override;
  std::size_t GetBlockGenerationWeight(Block const &previous, Address const &address) override;
  bool        ShouldGenerateBlock(Block const &previous, Address const &address) override;
  bool        ValidMinerForBlock(Block const &previous, Address const &address) override;
  /// @}

  // Accessors
  StakeUpdateQueue &      update_queue();
  StakeUpdateQueue const &update_queue() const;
  std::size_t             committee_size() const;

  CommitteePtr                         GetCommittee(Block const &previous);
  std::shared_ptr<StakeSnapshot const> GetCurrentStakeSnapshot() const;

  void Reset(StakeSnapshot const &snapshot, std::size_t committee_size);
  void Reset(StakeSnapshot &&snapshot, std::size_t committee_size);

  // Operators
  StakeManager &operator=(StakeManager const &) = delete;
  StakeManager &operator=(StakeManager &&) = delete;

private:
  static constexpr std::size_t HISTORY_LENGTH = 100;

  using BlockIndex       = uint64_t;
  using StakeSnapshotPtr = std::shared_ptr<StakeSnapshot>;
  using StakeHistory     = std::map<BlockIndex, StakeSnapshotPtr>;

  StakeSnapshotPtr LookupStakeSnapshot(BlockIndex block);
  void             ResetInternal(StakeSnapshotPtr &&snapshot, std::size_t committee_size);

  // Config & Components
  std::size_t                committee_size_{0};       ///< The "static" size of the committee
  EntropyGeneratorInterface &entropy_;                 ///< The reference to entropy module
  StakeUpdateQueue           update_queue_;            ///< The update queue of events
  StakeHistory               history_{};               ///< Cache of historical snapshots
  StakeSnapshotPtr           current_{};               ///< Most recent snapshot
  BlockIndex                 current_block_index_{0};  ///< Block index of most recent snapshot
};

inline std::size_t StakeManager::committee_size() const
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
