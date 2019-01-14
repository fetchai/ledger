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

#include <iostream>
#include <unordered_map>
#include "core/byte_array/byte_array.hpp"

#include "auctions/type_def.hpp"
#include "auctions/error_codes.hpp"
#include "auctions/item.hpp"
#include "auctions/bid.hpp"

namespace fetch {
namespace auctions {

// template <typename A, typename V = fetch::ml::Variable<A>>
class Auction
{
protected:
  // Auction parameters

  bool        smart_market_      = false;  // is it a smart market
  std::size_t max_items_         = 1;      // max items in auction
  std::size_t max_bids_          = 1;      // max bids per bidder per item
  std::size_t max_bids_per_item_ = std::numeric_limits<std::size_t>::max();  //
  std::size_t max_items_per_bid_ = 1;                                        //

  // records the block id on which this auction was born and will conclude
  BlockIdType start_block_ = std::numeric_limits<BlockIdType>::max();
  BlockIdType end_block_   = std::numeric_limits<BlockIdType>::max();

  ItemsContainerType items_{};
  std::vector<fetch::auctions::Bid>   bids_{};

  // a valid auction is ongoing (i.e. neither concluded nor yet to begin)
  bool auction_valid_ = false;

public:
  /**
   * constructor for an auction
   * @param start_block_id  defines the start time of an auction
   * @param end_block_id    defines the close time of an auction
   * @param item  defines the item to be sold
   * @param initiator  the id of the agent initiating the auction
   */
  Auction(BlockIdType start_block_id, BlockIdType end_block_id, bool smart_market = false,
          std::size_t max_bids = std::numeric_limits<std::size_t>::max())
    : smart_market_(smart_market)
    , max_bids_(max_bids)
    , start_block_(start_block_id)
    , end_block_(end_block_id)
  {
    // check on the length of the auction
    assert(end_block_ > start_block_);

    if (smart_market)
    {
      max_items_         = std::numeric_limits<std::size_t>::max();
      max_bids_per_item_ = std::numeric_limits<std::size_t>::max();
      max_items_per_bid_ = std::numeric_limits<std::size_t>::max();
    }

    // must be some items in the auction!
    assert(max_items_ > 0);

    auction_valid_ = true;
  }

  /**
   * returns every listed item
   * @return
   */
  std::vector<Item> ShowListedItems()
  {
    std::vector<Item> ret_vec{};
    for (auto &it : items_)
    {
      ret_vec.push_back(it.second);
    }
    return ret_vec;
  }

  /**
   * adds an item to the auction
   * @param item
   * @param item_owner
   * @param min_price
   * @return
   */
  ErrorCode AddItem(Item const &item)
  {
    if (!auction_valid_)
    {
      return ErrorCode::AUCTION_CLOSED;
    }

    // check if auction full
    if (items_.size() >= max_items_)
    {
      return ErrorCode::AUCTION_FULL;
    }

    // auction not full
    else
    {
      // check if item already listed
      if (items_.find(item.id) != items_.end())
      {
        return ErrorCode::ITEM_ALREADY_LISTED;  // item already listed error
        // TODO () - update trustworthiness of agent that try to double list items
      }

      // list the items
      else
      {
        items_.insert(std::pair<ItemIdType, Item>(item.id, item));
        return ErrorCode::SUCCESS;  // success
      }
    }
  }

  /**
   * Agent adds a bid (potentially on multiple items)
   * @param bid  a Bid object describing the items to bid on, the price, and the excluded items
   * @return
   */
  ErrorCode Bid(fetch::auctions::Bid bid)
  {
    // Check validity of bid and auction
    if (!auction_valid_)
    {
      return ErrorCode::AUCTION_CLOSED;
    }

    if (bid.items.size() > max_items_per_bid_)
    {
      return ErrorCode::TOO_MANY_ITEMS;
    }

    for (std::size_t j = 0; j < bid.items.size(); ++j)
    {
      // check item listed in auction
      if (!ItemInAuction(bid.items[j].id))
      {
        return ErrorCode::ITEM_NOT_LISTED;
      }

      // check the bidder has not exceeded their allowed number of bids on this item
      std::size_t n_bids = GetBidsCount(bid.bidder, bid.items[j].id);
      if (n_bids >= max_bids_)
      {
        return ErrorCode::TOO_MANY_BIDS;
      }
    }

    // place bids and update counter
    bids_.push_back(bid);
    for (std::size_t j = 0; j < bid.items.size(); ++j)
    {
      items_[bid.items[j].id].bids.push_back(bid);

      // count of how many times this bidder has bid on this item
      IncrementBidCount(bid.bidder, bid.items[j].id);
    }

    return ErrorCode::SUCCESS;
  }

  /**
   * Executes the auction by identifying the winners, and making appropriate transfers
   * @param current_block
   * @return
   */
  virtual bool Execute(BlockIdType current_block) = 0;

  AgentIdType Winner(ItemIdType item_id)
  {
    return items_[item_id].winner;
  }

  std::vector<AgentIdType> Winners()
  {
    std::vector<AgentIdType> winners{};
    for (auto &item_it : items_)
    {
      winners.push_back(item_it.second.winner);
    }
    return winners;
  }

  ItemsContainerType Items()
  {
    return items_;
  }

private:
  /**
   * checks whether item in the auction
   * @param item_id
   * @return
   */
  bool ItemInAuction(ItemIdType item_id)
  {
    ItemsContainerType::iterator item_it = items_.find(item_id);
    if (item_it == items_.end())
    {
      return false;
    }
    return true;
  }

  /**
   * gets the number of bids this bidder has previous placed on this item in this auction
   * if the item is not in the auction, returns 0
   * @param bidder
   * @param item_id
   * @return
   */
  std::size_t GetBidsCount(AgentIdType bidder, ItemIdType item_id)
  {
    if (ItemInAuction(item_id))
    {
      if (items_[item_id].agent_bid_count.find(bidder) == items_[item_id].agent_bid_count.end())
      {
        return 0;
      }
      return items_[item_id].agent_bid_count[bidder];
    }
    return 0;
  }

  /**
   * gets the number of bids on this item in this auction
   * @param bidder
   * @param item_id
   * @return
   */
  std::size_t GetBidsCount(ItemIdType item_id)
  {
    if (ItemInAuction(item_id))
    {
      return items_[item_id].bid_count;
    }
    return 0;
  }

  /**
   * update number of bids
   * @param bidder
   * @param item_id
   */
  void IncrementBidCount(AgentIdType bidder, ItemIdType item_id)
  {
    assert(ItemInAuction(item_id));

    items_[item_id].bid_count++;

    if (items_[item_id].agent_bid_count.find(bidder) == items_[item_id].agent_bid_count.end())
    {
      items_[item_id].agent_bid_count.insert(std::pair<AgentIdType, std::size_t>(bidder, 1));
    }
    else
    {
      ++items_[item_id].agent_bid_count[bidder];
    }
  }

  /**
   * finds the auction winners
   */
  virtual void SelectWinners() = 0;
};

}  // namespace auctions
}  // namespace fetch