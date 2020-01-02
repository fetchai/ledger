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

#include "chain/address.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "ledger/consensus/consensus_interface.hpp"
#include "ledger/consensus/stake_manager_interface.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/consensus/stake_update_queue.hpp"

#include <vector>

namespace fetch {

namespace crypto {
class Identity;
}

namespace ledger {

class Block;
class EntropyGeneratorInterface;
class StorageInterface;

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
 * can be used to deterministically build a cabinet.
 *
 */
class StakeManager final : public StakeManagerInterface
{
public:
  using Identity   = crypto::Identity;
  using Cabinet    = std::vector<Identity>;
  using CabinetPtr = std::shared_ptr<Cabinet const>;

  // Construction / Destruction
  StakeManager()                     = default;
  StakeManager(StakeManager const &) = delete;
  StakeManager(StakeManager &&)      = delete;
  ~StakeManager() override           = default;

  /// @name Stake Manager Interface
  /// @{
  void UpdateCurrentBlock(BlockIndex block_index) override;
  /// @}

  /// @name Committee Generation
  CabinetPtr BuildCabinet(Block const &current, uint64_t cabinet_size,
                          ConsensusInterface::Minerwhitelist const &whitelist = {});
  CabinetPtr BuildCabinet(uint64_t block_number, uint64_t entropy, uint64_t cabinet_size,
                          ConsensusInterface::Minerwhitelist const &whitelist = {}) const;
  /// @}

  /// @name Persistence
  /// @{
  bool Save(StorageInterface &storage);
  bool Load(StorageInterface &storage);
  /// @}

  // Accessors for the executor
  StakeUpdateQueue &      update_queue();
  StakeUpdateQueue const &update_queue() const;

  std::shared_ptr<StakeSnapshot const> GetCurrentStakeSnapshot() const;

  StakeManager::CabinetPtr Reset(StakeSnapshot const &snapshot, uint64_t cabinet_size);
  StakeManager::CabinetPtr Reset(StakeSnapshot &&snapshot, uint64_t cabinet_size);

  // Operators
  StakeManager &operator=(StakeManager const &) = delete;
  StakeManager &operator=(StakeManager &&) = delete;

private:
  static constexpr std::size_t HISTORY_LENGTH = 1000;

  using BlockIndex       = uint64_t;
  using StakeSnapshotPtr = std::shared_ptr<StakeSnapshot>;
  using StakeHistory     = std::map<BlockIndex, StakeSnapshotPtr>;

  StakeSnapshotPtr         LookupStakeSnapshot(BlockIndex block) const;
  StakeManager::CabinetPtr ResetInternal(StakeSnapshotPtr &&snapshot, uint64_t cabinet_size);

  StakeUpdateQueue update_queue_;            ///< The update queue of events
  StakeHistory     stake_history_{};         ///< Cache of historical snapshots
  StakeSnapshotPtr current_{};               ///< Most recent snapshot
  BlockIndex       current_block_index_{0};  ///< Block index of most recent snapshot

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

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

namespace serializers {

template <typename D>
struct MapSerializer<ledger::StakeManager, D>
{
public:
  using Type       = ledger::StakeManager;
  using DriverType = D;

  static uint8_t const UPDATE_QUEUE        = 1;
  static uint8_t const STAKE_HISTORY       = 2;
  static uint8_t const CURRENT_SNAPSHOT    = 3;
  static uint8_t const CURRENT_BLOCK_INDEX = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &stake_manager)
  {
    auto map = map_constructor(5);
    map.Append(UPDATE_QUEUE, stake_manager.update_queue_);
    map.Append(STAKE_HISTORY, stake_manager.stake_history_);
    map.Append(CURRENT_SNAPSHOT, stake_manager.current_);
    map.Append(CURRENT_BLOCK_INDEX, stake_manager.current_block_index_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &stake_manager)
  {
    map.ExpectKeyGetValue(UPDATE_QUEUE, stake_manager.update_queue_);
    map.ExpectKeyGetValue(STAKE_HISTORY, stake_manager.stake_history_);
    map.ExpectKeyGetValue(CURRENT_SNAPSHOT, stake_manager.current_);
    map.ExpectKeyGetValue(CURRENT_BLOCK_INDEX, stake_manager.current_block_index_);
  }
};

}  // namespace serializers

}  // namespace fetch
