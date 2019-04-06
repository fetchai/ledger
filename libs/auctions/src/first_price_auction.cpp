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

#include "auctions/first_price_auction.hpp"

namespace fetch {
namespace auctions {

FirstPriceAuction::FirstPriceAuction()
  : Auction(false, std::numeric_limits<std::size_t>::max())
{
  max_items_         = std::numeric_limits<std::size_t>::max();
  max_bids_          = std::numeric_limits<std::size_t>::max();
  max_items_per_bid_ = 1;
  max_bids_per_item_ = std::numeric_limits<std::size_t>::max();
}

ErrorCode FirstPriceAuction::Execute()
{
  if (!(auction_valid_ == AuctionState::LISTING))
  {
    return ErrorCode::AUCTION_CLOSED;
  }

  assert(max_items_per_bid_ == 1);

  // pick winning bid
  SelectWinners();

  // deduct funds from winner

  // transfer item to winner

  // close auction
  auction_valid_ = AuctionState::CLEARED;

  return ErrorCode::SUCCESS;
}

/**
 * finds the highest bid on each item
 */
void FirstPriceAuction::SelectWinners()
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

}  // namespace auctions
}  // namespace fetch
