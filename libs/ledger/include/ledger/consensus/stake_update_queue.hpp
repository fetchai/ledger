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
#include "core/synchronisation/protected.hpp"
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

  Protected<BlockUpdates> updates_{};  ///< The update queue

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

/**
 * Visit the underlying queue container directly
 *
 * @tparam Visitor The type of the visitor functor
 * @param visitor The visiting functor
 */
template <typename Visitor>
void StakeUpdateQueue::VisitUnderlyingQueue(Visitor &&visitor)
{
  updates_.ApplyVoid([&](BlockUpdates &updates) { visitor(updates); });
}

}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::StakeUpdateQueue, D>
{
public:
  using Type       = ledger::StakeUpdateQueue;
  using DriverType = D;

  static uint8_t const BLOCK_UPDATES = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &stake_manager)
  {
    auto map = map_constructor(1);
    stake_manager.updates_.ApplyVoid(
        [&map](Type::BlockUpdates const &updates) { map.Append(BLOCK_UPDATES, updates); });
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &stake_manager)
  {
    stake_manager.updates_.ApplyVoid(
        [&map](Type::BlockUpdates &updates) { map.ExpectKeyGetValue(BLOCK_UPDATES, updates); });
  }
};

}  // namespace serializers

}  // namespace fetch
