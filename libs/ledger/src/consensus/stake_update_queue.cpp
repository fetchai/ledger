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

#include "ledger/consensus/stake_tracker.hpp"
#include "ledger/consensus/stake_update_queue.hpp"

namespace fetch {
namespace ledger {

/**
 * Applies all relevant updates to the specified tracker for the given block index
 *
 * @param block_index The block index that is being triggered
 * @param tracker The tracker to be updated
 */
void StakeUpdateQueue::ApplyUpdates(BlockIndex block_index, StakeTracker &tracker)
{
  // make sure that the next block in the map is in fact the correct block index
  if (!updates_.empty())
  {
    auto next_update_it = updates_.begin();
    if (block_index < next_update_it->first)
    {
      // no update to be applied
      return;
    }
    else
    {
      // find the next appropriate update
      next_update_it = updates_.find(block_index);

      // ensure that this is the next block to be updated
      if (next_update_it->first == block_index)
      {
        // apply all the updates to the specified tracker
        for (auto const &element : next_update_it->second)
        {
          tracker.UpdateStake(element.first, element.second);
        }

        // advance the iterator along so it points to be new next update
        ++next_update_it;
      }

      // remove all the redundant entries
      updates_.erase(updates_.begin(), next_update_it);
    }
  }
}

} // namespace ledger
} // namespace fetch
