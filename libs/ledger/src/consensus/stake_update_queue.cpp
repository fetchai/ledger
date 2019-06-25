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

#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/consensus/stake_update_queue.hpp"

namespace fetch {
namespace ledger {

/**
 * Applies all relevant updates to the specified tracker for the given block index
 *
 * @param block_index The block index that is being triggered
 * @param tracker The tracker to be updated
 */
bool StakeUpdateQueue::ApplyUpdates(BlockIndex block_index, StakeSnapshotPtr const &reference,
                                    StakeSnapshotPtr &next)
{
  bool new_snapshot{false};

  updates_.Apply([&](BlockUpdates &updates) {
    // ensure the output is empty (this should always be the case anyway)
    next.reset();

    // make sure that the next block in the map is in fact the correct block index
    if (!updates.empty())
    {
      auto next_update_it = updates.begin();
      if (block_index < next_update_it->first)
      {
        // no update to be applied
        return;
      }
      else
      {
        // find the next appropriate update
        next_update_it = updates.find(block_index);

        // helpful references
        BlockIndex const &next_update_block_index = next_update_it->first;
        StakeMap const &  next_update_stake_map   = next_update_it->second;

        // ensure that this is the next block to be updated
        if (next_update_block_index == block_index)
        {
          // this should always be the case currently:
          if (!next_update_stake_map.empty())
          {
            // make a full copy of the stake snapshot
            next         = std::make_shared<StakeSnapshot>(*reference);
            new_snapshot = true;
          }

          // apply all the updates to the specified tracker
          for (auto const &element : next_update_stake_map)
          {
            next->UpdateStake(element.first, element.second);
          }

          // advance the iterator along so it points to be new next update
          ++next_update_it;
        }

        // remove all the redundant entries
        updates.erase(updates.begin(), next_update_it);
      }
    }
  });

  return new_snapshot;
}

}  // namespace ledger
}  // namespace fetch
