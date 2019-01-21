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

  using RandomInt = typename fetch::random::LinearCongruentialGenerator::random_type;

public:
  CombinatorialAuction(BlockId start_block_id, BlockId end_block_id, std::uint32_t max_flips = 3)
    : Auction(start_block_id, end_block_id, true, std::numeric_limits<std::size_t>::max())
    , max_flips_(max_flips)
  {
    max_items_         = std::numeric_limits<std::size_t>::max();
    max_bids_          = std::numeric_limits<std::size_t>::max();
    max_items_per_bid_ = std::numeric_limits<std::size_t>::max();
    max_bids_per_item_ = std::numeric_limits<std::size_t>::max();
  }

  void                               BuildGraph();
  void                               SelectBid(std::size_t const &bid);
  Value                              TotalBenefit();
  bool                               Execute(BlockId current_block) override;
  fetch::math::linalg::Matrix<Value> Couplings();
  fetch::math::ShapelessArray<Value> LocalFields();
  std::uint32_t                      Active(std::size_t n);
  void                               Mine(std::size_t random_seed, std::size_t run_time);
  ErrorCode                          PlaceBid(Bid const &bid);
  ErrorCode                          AddItem(Item const &item);

private:
  // bids on binary vector
  fetch::math::linalg::Matrix<Value>         couplings_;
  fetch::math::ShapelessArray<Value>         local_fields_;
  fetch::math::ShapelessArray<std::uint32_t> active_;
  fetch::math::ShapelessArray<std::uint32_t> prev_active_;

  std::uint32_t max_flips_ = std::numeric_limits<std::uint32_t>::max();

  bool graph_built_ = false;

  void SelectWinners() override;
};

/**
 * adding new items automatically sets graph built back to false. This is useful if items and bids
 * are added, some mining takes place, and then more items are added later
 * @param item
 * @return
 */
ErrorCode CombinatorialAuction::AddItem(Item const &item)
{
  graph_built_ = false;
  return Auction::AddItem(item);
}

/**
 * adding new items automatically sets graph built back to false. This is useful if items and bids
 * are added, some mining takes place, and then more bids are added later
 * @param item
 * @return
 */
ErrorCode CombinatorialAuction::PlaceBid(Bid const &bid)
{
  graph_built_ = false;
  return Auction::PlaceBid(bid);
}

/**
 * mining function for finding better solutions
 */
void CombinatorialAuction::Mine(std::size_t random_seed, std::size_t run_time)
{
  fetch::random::LaggedFibonacciGenerator<> rng(random_seed);
  BuildGraph();

  for (std::size_t j = 0; j < active_.size(); ++j)
  {
    RandomInt val = (rng() >> 17) & 1;
    active_[j]    = static_cast<std::uint32_t>(val);
  }

  // simulated annealing
  Value beta_start = 0.01;
  Value beta_end   = 1;
  Value db         = (beta_end - beta_start) / static_cast<Value>(run_time);
  Value beta       = beta_start;
  Value prev_reward, new_reward, de;

  std::size_t rejected = 0;

  for (std::size_t i = 0; i < run_time; ++i)
  {
    for (std::size_t j = 0; j < bids_.size(); ++j)
    {
      prev_active_.Copy(active_);
      prev_reward = TotalBenefit();

      RandomInt nn = 1 + ((rng() >> 17) % max_flips_);
      for (RandomInt k = 0; k < nn; ++k)
      {
        RandomInt n = (rng() >> 17) % bids_.size();

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

      Value ran_val   = static_cast<Value>(rng.AsDouble());
      Value threshold = std::exp(-beta * de);  // TODO(tfr): use exponential approximation

      if (ran_val >= threshold)
      {
        active_.Copy(prev_active_);
        rejected += 1;
      }
    }

    // annealing
    beta += db;
  }
}

std::uint32_t CombinatorialAuction::Active(std::size_t n)
{
  assert(graph_built_);
  return active_[n];
}

fetch::math::ShapelessArray<Value> CombinatorialAuction::LocalFields()
{
  return local_fields_;
}

fetch::math::linalg::Matrix<Value> CombinatorialAuction::Couplings()
{
  return couplings_;
}

bool CombinatorialAuction::Execute(BlockId current_block)
{
  if ((end_block_ == current_block) && auction_valid_)
  {
    SelectWinners();
    auction_valid_ = false;
    return true;
  }
  return false;
}

/**
 * total benefit calculate the same was as energy in simulated annealing, i.e. :
 * E = Sum(couplings_ * a1 * a2) + Sum(local_fields_ * a1)
 * @return  returns a reward value
 */
Value CombinatorialAuction::TotalBenefit()
{
  assert(graph_built_);

  Value         reward = 0;
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

void CombinatorialAuction::SelectBid(std::size_t const &bid)
{
  if (active_.size() != bids_.size())
  {
    active_.Resize(bids_.size());
  }

  for (std::size_t j = 0; j < bids_.size(); ++j)
  {
    if (couplings_(bid, j) != 0)
    {
      active_[j] = 0;
    }
  }

  active_[bid] = 1;
}

/**
 * couplings_ = Sum(Bi + Bj) + delta
 */
void CombinatorialAuction::BuildGraph()
{
  couplings_    = fetch::math::linalg::Matrix<Value>::Zeroes({bids_.size(), bids_.size()});
  local_fields_ = fetch::math::ShapelessArray<Value>::Zeroes({bids_.size()});
  active_       = fetch::math::ShapelessArray<std::uint32_t>::Zeroes({bids_.size()});

  for (std::size_t i = 0; i < bids_.size(); ++i)
  {

    // set local fields according to the following equation:
    // local_fields_ = bid_price - Sum(items.min_price)
    // thus local_fields_ represents the release value due to a bid
    // and only bids with positive local_fields can be accepted
    local_fields_[i] = static_cast<Value>(bids_[i].price);
    couplings_.Set(i, i, 0);
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
  }

  for (std::size_t i = 0; i < bids_.size(); ++i)
  {

    Value max_local_fields_;
    max_local_fields_           = fetch::math::Max(local_fields_);
    Value exclusive_bid_penalty = 2 * max_local_fields_;

    // next set couplings strengths
    for (std::size_t j = i + 1; j < bids_.size(); ++j)
    {
      Value coupling = 0;

      // penalize exclusive bid combinations
      for (std::size_t k = 0; k < bids_[j].excludes.size(); ++k)
      {
        if (bids_[j].excludes[k].id == bids_[i].id)
        {
          coupling = exclusive_bid_penalty;
        }
      }

      for (std::size_t k = 0; k < items_.size(); ++k)
      {
        ItemId cur_id = items_[k].id;

        for (auto const &itm1 : bids_[i].items)
        {
          for (auto const &itm2 : bids_[j].items)
          {
            if ((cur_id == itm1.id) && (cur_id == itm2.id))
            {
              coupling += (bids_[i].price + bids_[j].price);
            }
          }
        }
      }

      couplings_.At(i, j) = couplings_.At(j, i) = -coupling;
    }
  }

  graph_built_ = true;
}

/**
 * finds the highest bid on each item
 */
void CombinatorialAuction::SelectWinners()
{
  // assign all item winners according to active bids
  std::vector<BidId> ret{};
  for (std::size_t j = 0; j < active_.size(); ++j)
  {
    if (active_[j] == 1)
    {
      for (auto &item : bids_[j].items)
      {
        item.winner = bids_[j].id;
      }
    }
  }
}
}  // namespace auctions
}  // namespace fetch
