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

#include <random>

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
    double beta_start = 0.01;
    double beta_end   = 1;
    double db         = (beta_end - beta_start) / run_time;
    double beta       = beta_start;
    double prev_reward, new_reward, de;

    std::size_t rejected = 0;

    std::random_device                         rd{};
    std::mt19937                               gen{rd()};
    std::uniform_int_distribution<std::size_t> d(1, 3);
    std::uniform_int_distribution<std::size_t> r_bids(0, bids_.size() - 1);
    std::uniform_real_distribution<double>     zero_to_one(0.0, 1.0);

    for (std::size_t i = 0; i < run_time; ++i)
    {
      for (std::size_t j = 0; j < bids_.size(); ++j)
      {
        prev_active_ = active_;
        prev_reward  = TotalBenefit();

        auto nn = d(gen);
        for (std::size_t k = 0; k < nn; ++k)
        {
          auto n = r_bids(gen);

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

        if (static_cast<double>(zero_to_one(gen)) >= std::exp(-beta * de))
        {
          active_ = prev_active_;
          rejected += 1;
        }
      }

      beta += db;
    }

    std::cout << "TotalBenefit(): " << TotalBenefit() << std::endl;
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
  fetch::math::linalg::Matrix<double>        couplings_;
  fetch::math::ShapelessArray<double>        local_fields_;
  fetch::math::ShapelessArray<std::uint32_t> active_;
  fetch::math::ShapelessArray<std::uint32_t> prev_active_;


  /**
   * couplings_ = Sum(Bi + Bj) + delta
   */
  void BuildGraph()
  {
    couplings_    = fetch::math::linalg::Matrix<double>::Zeroes({items_.size(), items_.size()});
    local_fields_ = fetch::math::ShapelessArray<double>::Zeroes({items_.size()});
    active_       = fetch::math::ShapelessArray<std::uint32_t>::Zeroes({items_.size()});

    for (std::size_t i = 0; i < bids_.size(); ++i)
    {

      // set local fields according to the following equation:
      // local_fields_ = bid_price - Sum(items.min_price)
      // thus local_fields_ represents the release value due to a bid
      // and only bids with positive local_fields can be accepted
      local_fields_[i] = bids_[i].price;
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

      double max_local_fields_;
      fetch::math::Max(local_fields_, max_local_fields_);
      double exclusive_bid_penalty = 2 * max_local_fields_;

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
  double TotalBenefit()
  {
    double        reward = 0;
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
    // TODO () can just iterate through active_ to identify selected bids
  }
};

}  // namespace auctions
}  // namespace fetch
