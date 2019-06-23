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

class StakeManager : public StakeManagerInterface
{
public:
  using Committee = std::vector<Address>;

  // Construction / Destruction
  explicit StakeManager(EntropyGeneratorInterface &entropy);
  StakeManager(StakeManager const &) = delete;
  StakeManager(StakeManager &&) = delete;
  ~StakeManager() override = default;

  /// @name Stake Manager Interface
  /// @{
  Validity Validate(Block const &block) const override;
  void UpdateCurrentBlock(Block const &current) override;
  bool ShouldGenerateBlock(Block const &previous) override;
  /// @}

  StakeUpdateQueue &update_queue();
  StakeUpdateQueue const &update_queue() const;

  Committee const &GetCommittee(Block const &previous);
  std::size_t GetBlockGenerationWeight(Block const &previous, Address const &address);

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

  void ResetInternal(StakeSnapshotPtr &&snapshot, std::size_t committee_size);

  EntropyGeneratorInterface &entropy_;

  StakeUpdateQueue update_queue_;

  Committee cached_committee_;

  std::size_t committee_size_{0};
  StakeHistory history_{};
  StakeSnapshotPtr current_{};
};

inline StakeUpdateQueue &StakeManager::update_queue()
{
  return update_queue_;
}

inline StakeUpdateQueue const &StakeManager::update_queue() const
{
  return update_queue_;
}

} // namespace ledger
} // namespace fetch
