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

#include "core/digest.hpp"
#include "core/random/lcg.hpp"
#include "ledger/consensus/stake_snapshot.hpp"

#include <algorithm>
#include <unordered_set>

namespace fetch {
namespace ledger {

using Identity    = crypto::Identity;
using IdentitySet = std::unordered_set<Identity>;
using DRNG        = random::LinearCongruentialGenerator;

/**
 * Given the source of entropy, generate a selection of stakes identities based on proportional
 * probability against stakes.
 *
 * @param entropy The seed source of entropy
 * @param count The size of the selection
 * @return The selection of identities
 */
StakeSnapshot::CabinetPtr StakeSnapshot::BuildCabinet(
    uint64_t entropy, std::size_t count,
    std::set<byte_array::ConstByteArray> const &whitelist) const
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Building cabinet from pool of: ", stake_index_.size());

  CabinetPtr cabinet = std::make_shared<Cabinet>();
  cabinet->reserve(count);
  auto stake_index = stake_index_;  // Since build cabinet is const

  // Pre filter against the whitelist if necessary
  if (!whitelist.empty())
  {
    for (auto it = stake_index.begin(); it != stake_index.end();)
    {
      if (whitelist.find((*it)->identity.identifier()) == whitelist.end())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Removing staker since not in whitelist: ",
                       (*it)->identity.identifier().ToBase64());
        it = stake_index.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  if (count >= identity_index_.size())
  {
    for (auto const &record : stake_index)
    {
      cabinet->emplace_back(record->identity);
    }
  }
  else
  {
    IdentitySet chosen_identities;
    DRNG        rng(entropy);

    // ensure the stake list is reset to a deterministic state
    std::sort(stake_index.begin(), stake_index.end(),
              [](RecordPtr const &a, RecordPtr const &b) { return a->identity < b->identity; });

    for (std::size_t i = 0; i < count; ++i)
    {
      // shuffle the array
      std::shuffle(stake_index.begin(), stake_index.end(), rng);

      // make the selection
      uint64_t selection = rng() % total_stake_;

      std::size_t index{0};
      for (auto const &record : stake_index)
      {
        if (record->stake >= selection)
        {
          // TODO(issue 1247): This ensures in the case of a collision, the next item in the list is
          //                   picked. However, there is an edge case here when this selection is at
          //                   the  end of the stake_index array. In this case the output will
          //                   contain fewer items.
          selection = 0;

          if (chosen_identities.find(record->identity) == chosen_identities.end())
          {
            cabinet->emplace_back(record->identity);
            chosen_identities.emplace(record->identity);

            // exit from the search loop
            break;
          }
        }
        else  // (record->stake < selection)
        {
          selection -= record->stake;
        }

        ++index;
      }
    }
  }

  return cabinet;
}

/**
 * Look up stake for a given identity
 *
 * @param identity The identity to be queried
 * @return The stake amount
 */
uint64_t StakeSnapshot::LookupStake(Identity const &identity) const
{
  uint64_t stake{0};

  auto const it = identity_index_.find(identity);
  if (it != identity_index_.end())
  {
    stake = it->second->stake;
  }

  return stake;
}

/**
 * Update the stake for a specified identity
 *
 * To delete someone's stake simply set the stake to zero
 *
 * @param identity The identity to be updates
 * @param stake The new absolute value of stake
 */
void StakeSnapshot::UpdateStake(Identity const &identity, uint64_t stake)
{
  auto it = identity_index_.find(identity);
  if (it == identity_index_.end())
  {
    // new stake
    auto record = std::make_shared<Record>(Record{identity, stake});

    // update all the indexes
    identity_index_[identity] = record;
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
    if (stake == 0)
    {
      // special case - removed from staking pool

      // update the total stake
      total_stake_ -= it->second->stake;

      // remove the from identity index
      identity_index_.erase(it);

      // remove from the stake index
      auto const last = std::remove_if(
          stake_index_.begin(), stake_index_.end(),
          [&identity](RecordPtr const &record) { return identity == record->identity; });
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

}  // namespace ledger
}  // namespace fetch
