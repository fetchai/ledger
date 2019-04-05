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
#include "math/tensor.hpp"

#include "core/random/lcg.hpp"
#include "core/random/lfg.hpp"

namespace fetch {
namespace auctions {

class CombinatorialAuction : public Auction
{

  using RandomInt = typename fetch::random::LinearCongruentialGenerator::random_type;

public:
  explicit CombinatorialAuction(std::uint32_t max_flips = 3)
    : Auction(true, std::numeric_limits<std::size_t>::max())
    , best_value_(std::numeric_limits<Value>::lowest())
    , max_flips_(max_flips)
  {
    max_items_         = std::numeric_limits<std::size_t>::max();
    max_bids_          = std::numeric_limits<std::size_t>::max();
    max_items_per_bid_ = std::numeric_limits<std::size_t>::max();
    max_bids_per_item_ = std::numeric_limits<std::size_t>::max();
  }

  void                       BuildGraph();
  void                       SelectBid(std::size_t const &bid);
  Value                      TotalBenefit();
  fetch::math::Tensor<Value> Couplings();
  ErrorCode                  Execute() override;
  fetch::math::Tensor<Value> LocalFields();
  std::uint32_t              Active(std::size_t n);
  void                       Mine(std::size_t random_seed, std::size_t run_time);
  ErrorCode                  PlaceBid(Bid const &bid);
  ErrorCode                  AddItem(Item const &item);
  ErrorCode                  ShowAuctionResult();

private:
  // bids on binary vector
  fetch::math::Tensor<Value>         couplings_;
  fetch::math::Tensor<Value>         local_fields_;
  fetch::math::Tensor<std::uint32_t> active_;
  fetch::math::Tensor<std::uint32_t> prev_active_;

  Value                              best_value_;
  fetch::math::Tensor<std::uint32_t> best_active_;

  std::uint32_t max_flips_ = std::numeric_limits<std::uint32_t>::max();

  bool graph_built_ = false;

  void SelectWinners() override;
};

}  // namespace auctions
}  // namespace fetch
