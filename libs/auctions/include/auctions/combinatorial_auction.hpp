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

#include "auctions/auction.hpp"
#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/linalg/matrix.hpp"

#include "core/random/lcg.hpp"
#include "core/random/lfg.hpp"

namespace fetch {
namespace auctions {

class CombinatorialAuction : public Auction
{
public:
  CombinatorialAuction(BlockIdType start_block_id, BlockIdType end_block_id)
    : Auction(start_block_id, end_block_id, true, std::numeric_limits<std::size_t>::max())
  {
    max_items_         = std::numeric_limits<std::size_t>::max();
    max_bids_          = std::numeric_limits<std::size_t>::max();
    max_items_per_bid_ = std::numeric_limits<std::size_t>::max();
    max_bids_per_item_ = std::numeric_limits<std::size_t>::max();
  }

  /**
   * mining function for finding better solutions
   */
  void Mine(std::size_t random_seed, std::size_t run_time)
  {
    BuildGraph();

    // simulated annealing
    ValueType beta_start = 0.001;
    ValueType beta_end   = 1;
    ValueType db         = (beta_end - beta_start) / static_cast<ValueType>(run_time);
    ValueType beta       = beta_start;
    ValueType prev_reward, new_reward, de;

    std::size_t rejected = 0;

    fetch::random::LaggedFibonacciGenerator<>  int_gen(random_seed);
    fetch::random::LinearCongruentialGenerator real_gen;

    for (std::size_t i = 0; i < run_time; ++i)
    {
      for (std::size_t j = 0; j < bids_.size(); ++j)
      {
        prev_active_ = active_;
        prev_reward  = TotalBenefit();

        std::size_t nn = static_cast<std::size_t>(1 + int_gen() % bids_.size());
        for (std::size_t k = 0; k < nn; ++k)
        {
          std::size_t n = static_cast<std::size_t>(int_gen() % bids_.size());

          if (active_[n] == 1)
          {
            active_[n] = 0;
          }
          else
          {
            active_[n] = 1;
          }
        }

        new_reward = TotalBenefit();
        de         = prev_reward - new_reward;

        std::cout << "de: " << de << std::endl;

        ValueType ran_val   = static_cast<ValueType>(real_gen.AsDouble());
        ValueType threshold = std::exp(-beta * de);

        if (ran_val >= threshold)
        {
          active_ = prev_active_;
          rejected += 1;
        }
        else
        {
          if (beta > 0.9 && (new_reward == 8))
          {
            std::cout << "TotalBenefit(): " << TotalBenefit() << std::endl;
          }
        }
      }

      // annealing
      beta += db;
    }

    std::cout << "Final - TotalBenefit(): " << TotalBenefit() << std::endl;
  }

  bool Execute(BlockIdType current_block)
  {
    if ((end_block_ == current_block) && auction_valid_)
    {
      // pick winning bid
      SelectWinners();

      // deduct funds from winner

      // transfer item to winner

      // TODO (handle defaulting to next highest bid if winner cannot pay)

      // close auction
      auction_valid_ = false;

      return true;
    }
    return false;
  }

  std::uint32_t Active(std::size_t n)
  {
    return active_[n];
  }

private:
  // bids on binary vector
  fetch::math::linalg::Matrix<ValueType>     couplings_;
  fetch::math::ShapelessArray<ValueType>     local_fields_;
  fetch::math::ShapelessArray<std::uint32_t> active_;
  fetch::math::ShapelessArray<std::uint32_t> prev_active_;

  /**
   * couplings_ = Sum(Bi + Bj) + delta
   */
  void BuildGraph()
  {
    couplings_    = fetch::math::linalg::Matrix<ValueType>::Zeroes({bids_.size(), bids_.size()});
    local_fields_ = fetch::math::ShapelessArray<ValueType>::Zeroes({bids_.size()});
    active_       = fetch::math::ShapelessArray<std::uint32_t>::Zeroes({bids_.size()});

    for (std::size_t i = 0; i < bids_.size(); ++i)
    {

      // set local fields according to the following equation:
      // local_fields_ = bid_price - Sum(items.min_price)
      // thus local_fields_ represents the release value due to a bid
      // and only bids with positive local_fields can be accepted
      local_fields_[i] = static_cast<ValueType>(bids_[i].price);
      for (auto &cur_item : items_)
      {
        for (std::size_t j = 0; j < bids_[i].items.size(); ++j)
        {
          if (bids_[i].items[j].id == cur_item.second.id)
          {
            local_fields_[i] -= cur_item.second.min_price;
          }
        }
      }

      ValueType max_local_fields_;
      fetch::math::Max(local_fields_, max_local_fields_);
      ValueType exclusive_bid_penalty = 2 * max_local_fields_;

      // next set couplings strengths
      for (std::size_t j = i + 1; j < bids_.size(); ++j)
      {
        couplings_.At(i, j);

        // penalize exclusive bid combinations
        for (std::size_t k = 0; k < bids_[j].excludes.size(); ++k)
        {
          if (bids_[j].excludes[k].id == bids_[i].id)
          {
            couplings_.Set(i, j, exclusive_bid_penalty);
          }
        }

        for (auto &cur_item : items_)
        {
          bool in_bid1 = false;
          for (std::size_t k = 0; k < bids_[i].items.size(); ++k)
          {
            if (bids_[i].items[k].id == cur_item.second.id)
            {
              in_bid1 = true;
            }
          }
          if (in_bid1)
          {
            for (std::size_t k = 0; k < bids_[j].items.size(); ++k)
            {
              if (bids_[j].items[k].id == cur_item.second.id)
              {
                couplings_.At(i, j) += (bids_[i].price + bids_[j].price);
              }
            }
          }
        }

        couplings_.At(i, j) = couplings_.At(j, i) = -1 * couplings_.At(i, j);
      }
    }
  }

  /**
   * total benefit calculate the same was as energy in simulated annealing, i.e. :
   * E = Sum(couplings_ * a1 * a2) + Sum(local_fields_ * a1)
   * @return  returns a reward value
   */
  ValueType TotalBenefit()
  {
    ValueType     reward = 0;
    std::uint32_t a1 = 0, a2 = 0;
    for (std::size_t i = 0; i < bids_.size(); ++i)
    {
      a1 = active_[i];
      reward += a1 * local_fields_[i];

      for (std::size_t j = 0; j < bids_.size(); ++j)
      {
        a2 = active_[j];
        reward += a1 * a2 * couplings_.At(i, j);
      }
    }

    return reward;
  }

  /**
   * finds the highest bid on each item
   */
  void SelectWinners()
  {
    for (std::size_t i = 0; i < active_.size(); ++i)
    {
      std::cout << "active_[i]: " << active_[i] << std::endl;
    }
    // TODO () can just iterate through active_ to identify selected bids
  }
};

}  // namespace auctions
}  // namespace fetch
