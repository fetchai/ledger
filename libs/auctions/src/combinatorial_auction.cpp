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

#include "auctions/combinatorial_auction.hpp"
#include "math/matrix_operations.hpp"

namespace fetch {
namespace auctions {

/**
 * adding new items automatically sets graph built back to false. This is useful if items and bids
 * are added, some mining takes place, and then more items are added later
 * @param item
 * @return
 */
ErrorCode CombinatorialAuction::AddItem(Item const &item)
{
  ErrorCode ec = Auction::AddItem(item);
  if (ec == fetch::auctions::ErrorCode::SUCCESS)
  {
    graph_built_ = false;
  }
  return ec;
}

/**
 * adding new items automatically sets graph built back to false. This is useful if items and bids
 * are added, some mining takes place, and then more bids are added later
 * @param item
 * @return
 */
ErrorCode CombinatorialAuction::PlaceBid(Bid const &bid)
{
  ErrorCode ec = Auction::PlaceBid(bid);
  if (ec == fetch::auctions::ErrorCode::SUCCESS)
  {
    graph_built_ = false;
  }
  return ec;
}

/**
 * mining function for finding better solutions
 */
void CombinatorialAuction::Mine(std::size_t random_seed, std::size_t run_time)
{
  auction_valid_ = AuctionState::MINING;
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
    std::cout << "mining run: " << i << std::endl;
    for (std::size_t j = 0; j < bids_.size(); ++j)
    {
      prev_active_ = active_.Clone();
      prev_reward  = TotalBenefit();

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

      // record best iteration
      if (new_reward > best_value_)
      {
        best_active_ = active_.Clone();
        best_value_  = new_reward;
      }

      // stochastically ignore (or keep) new activation set
      de              = prev_reward - new_reward;
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

fetch::math::Tensor<Value> CombinatorialAuction::LocalFields()
{
  return local_fields_;
}

fetch::math::Tensor<Value> CombinatorialAuction::Couplings()
{
  return couplings_;
}

ErrorCode CombinatorialAuction::Execute()
{
  if (!(auction_valid_ == AuctionState::MINING))
  {
    return ErrorCode::AUCTION_CLOSED;
  }

  SelectWinners();
  auction_valid_ = AuctionState::CLEARED;
  return ErrorCode::SUCCESS;
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
      reward += a1 * a2 * couplings_.At({j, i});
    }
  }

  return reward;
}

void CombinatorialAuction::SelectBid(std::size_t bid)
{
  if (active_.size() != bids_.size())
  {
    active_ = fetch::math::Tensor<std::uint32_t>(bids_.size());
  }

  for (std::size_t j = 0; j < bids_.size(); ++j)
  {
    if (couplings_.At({j, bid}) != 0)
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
  couplings_    = fetch::math::Tensor<Value>({bids_.size(), bids_.size()});
  local_fields_ = fetch::math::Tensor<Value>(bids_.size());
  active_       = fetch::math::Tensor<std::uint32_t>(bids_.size());
  local_fields_.Fill(Value(0));
  active_.Fill(std::uint32_t(0));

  // check for any cases where bids specify to 'exclude all' and set up the relevant 'excludes'
  // vector
  for (std::size_t j = 0; j < bids_.size(); ++j)
  {
    if (bids_[j].exclude_all)
    {
      // get all bids made by this bidder (except current bid)
      std::vector<BidId> all_bids{};
      for (std::size_t k = 0; k < bids_.size(); ++k)
      {
        if (!(j == k) && (bids_[k].bidder == bids_[j].bidder))
        {
          all_bids.emplace_back(bids_[k].id);
        }
      }

      bids_[j].excludes = all_bids;
    }
  }

  // set local fields according to the following equation:
  // local_fields_ = bid_price - Sum(items.min_price)
  // thus local_fields_ represents the release value due to a bid
  // and only bids with positive local_fields can be accepted
  for (std::size_t i = 0; i < bids_.size(); ++i)
  {

    local_fields_[i] = static_cast<Value>(bids_[i].price);
    couplings_.Set({i, i}, 0);
    for (auto &cur_item : items_)
    {
      for (std::size_t j = 0; j < bids_[i].item_ids().size(); ++j)
      {
        if (bids_[i].item_ids()[j] == cur_item.second.id)
        {
          local_fields_[i] -= cur_item.second.min_price;
        }
      }
    }
  }

  // Assign Couplings
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
        if (bids_[j].excludes[k] == bids_[i].id)
        {
          coupling = exclusive_bid_penalty;
        }
      }

      for (auto const &itm : items_)
      {
        for (auto const &itm1 : bids_[i].item_ids())
        {
          for (auto const &itm2 : bids_[j].item_ids())
          {
            if ((itm.first == itm1) && (itm.first == itm2))
            {
              coupling += (bids_[i].price + bids_[j].price);
            }
          }
        }
      }

      couplings_.At({j, i}) = couplings_.At({i, j}) = -coupling;
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
    if (best_active_[j] == 1)
    {
      for (auto &item : items_)
      {
        for (auto &bid_item_id : bids_[j].item_ids())
        {
          if (bid_item_id == item.first)
          {
            item.second.winner     = bids_[j].id;
            item.second.sell_price = bids_[j].price;
          }
        }
      }
    }
  }
}

ErrorCode CombinatorialAuction::ShowAuctionResult()
{
  ErrorCode result = Auction::ShowAuctionResult();

  std::cout << "TotalBenefit(): " << TotalBenefit() << std::endl;

  return result;
}

}  // namespace auctions
}  // namespace fetch
