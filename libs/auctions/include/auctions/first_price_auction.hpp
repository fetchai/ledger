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

namespace fetch {
namespace auctions {

class FirstPriceAuction : public Auction
{
public:
  FirstPriceAuction(BlockIdType start_block_id, BlockIdType end_block_id)
    : Auction(start_block_id, end_block_id, false, std::numeric_limits<std::size_t>::max())
  {
    max_items_         = std::numeric_limits<std::size_t>::max();
    max_bids_          = std::numeric_limits<std::size_t>::max();
    max_items_per_bid_ = 1;
    max_bids_per_item_ = std::numeric_limits<std::size_t>::max();
  }

  bool Execute(BlockIdType current_block)
  {
    if ((end_block_ == current_block) && auction_valid_)
    {
      // TODO (multi-items-per-bid non-combinatorial auctions not implemented)
      assert(max_items_per_bid_ == 1);

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

private:
  /**
   * finds the highest bid on each item
   */
  void SelectWinners()
  {
    // iterate through all items in auction
    for (auto &cur_item_it : items_)
    {
      // find highest bid for this item and set winner
      for (auto &cur_bid_it : cur_item_it.second.bids)
      {
        if (cur_bid_it.price > cur_item_it.second.max_bid)
        {
          cur_item_it.second.winner     = cur_bid_it.bidder;
          cur_item_it.second.max_bid    = cur_bid_it.price;
          cur_item_it.second.sell_price = cur_bid_it.price;
        }
      }
    }
  }
};

}  // namespace auctions
}  // namespace fetch