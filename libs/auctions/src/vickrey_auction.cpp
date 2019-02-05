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

#include "auctions/vickrey_auction.hpp"

namespace fetch {
namespace auctions {

ErrorCode VickreyAuction::Execute()
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
void VickreyAuction::SelectWinners()
{
  // iterate through all items in auction
  for (auto &cur_item_it : items_)
  {
    // handle special cases of 0, 1, and 2 bids
    std::size_t loop_count = 0;

    // iterate through bids on this item
    if (cur_item_it.second.bids.size() > 2)
    {
      for (auto &cur_bid_it : cur_item_it.second.bids)
      {
        if (loop_count == 0)
        {
          cur_item_it.second.winner     = cur_bid_it.bidder;
          cur_item_it.second.max_bid    = cur_bid_it.price;
          cur_item_it.second.sell_price = cur_bid_it.price;
          ++loop_count;
        }
        else if (loop_count == 1)
        {
          if (cur_bid_it.price > cur_item_it.second.max_bid)
          {
            cur_item_it.second.winner  = cur_bid_it.bidder;
            cur_item_it.second.max_bid = cur_bid_it.price;
          }
          else
          {
            cur_item_it.second.sell_price = cur_bid_it.price;
          }
          ++loop_count;
        }
        else
        {
          if (cur_bid_it.price > cur_item_it.second.max_bid)
          {
            cur_item_it.second.winner     = cur_bid_it.bidder;
            cur_item_it.second.sell_price = cur_item_it.second.max_bid;
            cur_item_it.second.max_bid    = cur_bid_it.price;
          }
          else if (cur_bid_it.price > cur_item_it.second.max_bid)
          {
            cur_item_it.second.sell_price = cur_bid_it.price;
          }
        }
      }
    }

    // only 2 bids
    else if (cur_item_it.second.bids.size() == 2)
    {
      for (auto &cur_bid_it : cur_item_it.second.bids)
      {
        if (loop_count == 0)
        {
          // just using this to store the value for now
          cur_item_it.second.sell_price = cur_bid_it.price;

          cur_item_it.second.winner = cur_bid_it.bidder;
        }
        else if (loop_count == 1)
        {
          if (cur_bid_it.price > cur_item_it.second.sell_price)
          {
            cur_item_it.second.winner  = cur_bid_it.bidder;
            cur_item_it.second.max_bid = cur_bid_it.price;
          }
          else
          {
            cur_item_it.second.sell_price = cur_bid_it.price;
          }
        }
        ++loop_count;
      }
    }

    // only 1 bid
    else if (cur_item_it.second.bids.size() == 1)
    {
      for (auto &cur_bid_it : cur_item_it.second.bids)
      {
        cur_item_it.second.winner     = cur_bid_it.bidder;
        cur_item_it.second.sell_price = cur_bid_it.price;
        cur_item_it.second.max_bid    = cur_bid_it.price;
      }
    }

    // no bids!
    else
    {
    }
  }
}

}  // namespace auctions
}  // namespace fetch