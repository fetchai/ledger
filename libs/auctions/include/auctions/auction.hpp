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

//#include "ml/layers/layers.hpp"
#include <iostream>
//#include <memory>
#include <unordered_map>
//#include <unordered_set>
//#include <vector>
#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace auctions {

enum class ErrorCode
{
  SUCCESS,
  ITEM_ALREADY_LISTED,
  AUCTION_FULL,
  ITEM_NOT_LISTED,
  TOO_MANY_BIDS
};

struct Item;
using ItemIdType         = typename std::size_t;
using BlockIdType        = fetch::byte_array::ByteArray;
using ValueType          = typename std::size_t;
using AgentIdType        = typename std::size_t;
using ItemsContainerType = typename std::unordered_map<ItemIdType, Item>;

struct Item
{

  ItemIdType  id         = 0;
  AgentIdType seller_id  = 0;
  ValueType   min_price  = std::numeric_limits<ValueType>::max();
  ValueType   max_bid    = std::numeric_limits<ValueType>::min();
  ValueType   sell_price = std::numeric_limits<ValueType>::min();

  std::size_t bid_count = 0;
  AgentIdType winner    = 0;

  std::unordered_map<AgentIdType, ValueType>   bids{};
  std::unordered_map<AgentIdType, std::size_t> agent_bid_count{};
};

// template <typename A, typename V = fetch::ml::Variable<A>>
class Auction
{
protected:
  // Auction parameters

  std::size_t max_items_ = 0;  // max items in auction
  std::size_t max_bids_  = 1;  // max bids per bidder per item

  // records the block id on which this auction was born and will conclude
  BlockIdType start_block_ = std::numeric_limits<BlockIdType>::max();
  BlockIdType end_block_   = std::numeric_limits<BlockIdType>::max();

  ItemsContainerType items_{};

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
  Auction(BlockIdType start_block_id, BlockIdType end_block_id, std::size_t max_items)
    : max_items_(max_items)
    , start_block_(start_block_id)
    , end_block_(end_block_id)
  {
    // must be some items in the auction!
    assert(max_items_ > 0);

    // check on the length of the auction
    assert(end_block_ > start_block_);

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
  ErrorCode AddItem(ItemIdType const &item_id, AgentIdType const &seller_id,
                    ValueType const &min_price)
  {

    // check if auction full
    if (items_.size() >= max_items_)
    {
      return ErrorCode::AUCTION_FULL;
    }

    // auction not full
    else
    {
      // check if item already listed
      if (items_.find(item_id) != items_.end())
      {
        return ErrorCode::ITEM_ALREADY_LISTED;  // item already listed error
        // TODO () - update trustworthiness of agent that try to double list items
      }

      // list the items
      else
      {
        Item cur_item;
        cur_item.id        = item_id;
        cur_item.seller_id = seller_id;
        cur_item.min_price = min_price;

        items_.insert(std::pair<ItemIdType, Item>(item_id, cur_item));
        return ErrorCode::SUCCESS;  // success
      }
    }
  }

  /**
   * Agent adds a single bid for a single item
   * @param agent_id
   * @param bid
   * @param item_id
   * @return
   */
  ErrorCode AddSingleBid(ValueType bid, AgentIdType bidder, ItemIdType item_id)
  {

    // TODO - We don't save all previous bids. should we?

    // check item exists in auction
    if (!ItemInAuction(item_id))
    {
      return ErrorCode::ITEM_NOT_LISTED;
    }
    else
    {

      // count previous bid on item made by this bidder
      std::size_t n_bids = GetBidsCount(bidder, item_id);
      if (n_bids < max_bids_)
      {

        // update bid
        if (n_bids == 0)
        {
          items_[item_id].bids.insert(std::pair<AgentIdType, ValueType>(bidder, bid));
        }
        else
        {
          items_[item_id].bids[bidder] = bid;
        }

        // update bid counter
        IncrementBidCount(bidder, item_id);

        return ErrorCode::SUCCESS;
      }
      else
      {
        return ErrorCode::TOO_MANY_BIDS;
      }
    }
  }

  bool Execute(BlockIdType current_block)
  {
    if ((end_block_ == current_block) && auction_valid_)
    {

      // pick winning bids

      // for every item in the auction
      SelectWinners();

      std::cout << "Winners()[0]: " << Winners()[0] << std::endl;

      // deduct funds from winner

      // transfer item to winner

      // close auction
      auction_valid_ = false;

      return true;
    }
    return false;
  }

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
   * finds the highest bid on each item
   */
  virtual void SelectWinners()
  {
    // iterate through all items in auction
    for (auto &cur_item_it : items_)
    {
      // find highest bid for this item and set winner
      for (auto &cur_bid_it : cur_item_it.second.bids)
      {
        if (cur_bid_it.second > cur_item_it.second.max_bid)
        {
          cur_item_it.second.winner     = cur_bid_it.first;
          cur_item_it.second.max_bid    = cur_bid_it.second;
          cur_item_it.second.sell_price = cur_bid_it.second;
        }
      }
    }
  }
};

}  // namespace auctions
}  // namespace fetch

//
//    def BuildGraph(self):
//        # Building graph
//        self.item_winner = [(-1, -1) for i in range(len(self.items))]
//        self.active_bid = [ [False for j in range(len(self.bids[i]))] for i in
//        range(len(self.bids))]
//
//    def ClearBid(self, bid):
//        bid, n = bid
//
//        if bid == -1: return
//        if not self.active_bid[bid][n]: return
//
//        b = self.bids[bid][n]
//
//        for item, price in b:
//            self.item_winner[item] = (-1, -1)
//
//        self.active_bid[bid][n] = False
//
//    def SelectBid(self, bid):
//        bid, n = bid
//        for g in range(len(self.bids[bid])):
//            self.ClearBid( (bid,g) )
//
//        b = self.bids[bid][n]
//
//        cleared_bids = []
//        for item, price in b:
//            old_bid = self.item_winner[item]
//            cleared_bids.append(old_bid)
//
//        for old_bid in set(cleared_bids):
//            self.ClearBid(old_bid)
//
//        for item, price in b:
//            self.item_winner[item] = (bid, n)
//
//        self.active_bid[bid][n] = True
//
//    def TotalBenefit(self):
//        item_price = []
//        for i,n in set(self.item_winner):
//            if i == -1: continue
//            assert(self.active_bid[i][n])
//            item_price += self.bids[i][n]
//
//        reward = 0
//        for item, price in item_price:
//            name, l = self.items[item]
//            reward += price - l
//
//        return reward
//
//    def Mine(self, hash_value, runtime):
//        random.seed(hash_value)
//        self.BuildGraph()
//        beta_start = 0.01
//        beta_end = 0.3
//        db = (beta_end-beta_start)/runtime
//        beta = beta_start
//
//        for i in range(runtime):
//            oldstate = copy.copy(self.item_winner)
//            oldactive = copy.deepcopy(self.active_bid)
//            oldreward = self.TotalBenefit()
//
//            n = random.randint(0, len(self.bids)-1)
//            g = random.randint(0, len(self.bids[n])-1)
//
//            self.SelectBid( (n,g) )
//            newreward = self.TotalBenefit()
//            de = oldreward - newreward
//#            print(de,"=>",math.exp(beta*de))
//            if random.random() < math.exp(beta*de):
//                self.item_winner = oldstate
//                self.active_bid = oldactive
//
//            beta += db