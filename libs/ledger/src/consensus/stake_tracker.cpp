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

#include "core/random/lcg.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/consensus/stake_tracker.hpp"

#include <algorithm>
#include <unordered_set>

namespace fetch {
namespace ledger {

using AddressSet = std::unordered_set<Address>;
using DRNG       = random::LinearCongruentialGenerator;

/**
 * Given the source of entropy, generate a selection of stakes addresses based on proportional
 * probability against stakes.
 *
 * @param entropy The seed source of entropy
 * @param count The size of the selection
 * @return The selection of addresses
 */
StakeTracker::AddressArray StakeTracker::Sample(uint64_t entropy, std::size_t count)
{
  AddressArray addresses;
  addresses.reserve(count);

  {
    FETCH_LOCK(lock_);
    if (count >= address_index_.size())
    {
      for (auto const &record : stake_index_)
      {
        addresses.emplace_back(record->address);
      }
    }
    else
    {
      AddressSet chosen_addresses;
      DRNG rng(entropy);

      // ensure the stake list is reset to a deterministic state
      std::sort(stake_index_.begin(), stake_index_.end(), [](RecordPtr const &a, RecordPtr const &b) {
        return a->address.address() < b->address.address();
      });

      for (std::size_t i = 0; i < count; ++i)
      {
        // shuffle the array
        std::shuffle(stake_index_.begin(), stake_index_.end(), rng);

        // make the selection
        uint64_t selection = rng() % total_stake_;

        std::size_t index{0};
        for (auto const &record : stake_index_)
        {
          if (record->stake >= selection)
          {
            // TODO(XXX) This ensures in the case of a collision, the next item in the list is
            //           picked. However, there is an edge case here when this selection is at the
            //           end of the stake_index_ array. In this case the output will contain fewer
            //           items.
            selection = 0;

            if (chosen_addresses.find(record->address) == chosen_addresses.end())
            {
              addresses.emplace_back(record->address);
              chosen_addresses.emplace(record->address);

              // exit from the search loop
              break;
            }
          }
          else // (record->stake < selection)
          {
            selection -= record->stake;
          }

          ++index;
        }
      }
    }
  }

  return addresses;
}

/**
 * Lookup stake for a given address
 *
 * @param address The address to be queried
 * @return The stake amount
 */
uint64_t StakeTracker::LookupStake(Address const &address) const
{
  FETCH_LOCK(lock_);

  uint64_t stake{0};

  auto const it = address_index_.find(address);
  if (it != address_index_.end())
  {
    stake = it->second->stake;
  }

  return stake;
}

/**
 * Update the stake for a specified address
 *
 * To delete someones stake simply set the stake to zero
 *
 * @param address The address to be updates
 * @param stake The new absolute value of stake
 */
void StakeTracker::UpdateStake(Address const &address, uint64_t stake)
{
  FETCH_LOCK(lock_);

  auto it = address_index_.find(address);
  if (it == address_index_.end())
  {
    // new stake
    auto record = std::make_shared<Record>(Record{address, stake});

    // update all the indexes
    address_index_[address] = record;
    stake_index_.emplace_back(std::move(record));
    total_stake_ += stake;
  }
  else
  {
    // change in stake

    if (stake == it->second->stake)
    {
      // special case - no change in stake
      return;
    }
    else if (stake == 0)
    {
      // special case - removed from staking pool

      // update the total stake
      total_stake_ -= it->second->stake;

      // remove the from address index
      address_index_.erase(it);

      // remove from the stake index
      auto const last = std::remove_if(
          stake_index_.begin(), stake_index_.end(),
          [&address](RecordPtr const &record) { return address == record->address; });
      stake_index_.erase(last, stake_index_.end());
    }
    else
    {
      // normal change in stake

      if (stake >= it->second->stake)
      {
        // increase in stake
        total_stake_ += (stake - it->second->stake);
      }
      else
      {
        // decrease in stake
        total_stake_ -= (it->second->stake - stake);
      }

      // now update the stake
      it->second->stake = stake;
    }
  }
}

} // namespace ledger
} // namespace fetch