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

// template <typename A, typename V = fetch::ml::Variable<A>>
class Auction
{
public:
  using BlockIdType = typename std::size_t;

  using ValueType   = typename std::size_t;
  using AgentIdType = typename std::size_t;
  using ItemIdType  = typename std::size_t;

  using ItemsContainerType = typename std::unordered_map<ItemIdType, AgentIdType>;

  // contains multiple agents' bids
  using BidsContainerType = typename std::unordered_map<AgentIdType, ValueType>;

  // contains information on the item id, the owner id, and all bids
  using ItemBidsType = typename std::unordered_map<ItemIdType, BidsContainerType>;

  using BidCountType = typename std::unordered_map<AgentIdType, std::size_t>;

  using ItemBidCountType = typename std::unordered_map<ItemIdType, BidCountType>;
private:
  // Auction parameters

  std::size_t max_items_ = 0;  // max items in auction
  std::size_t max_bids_  = 1;  // max bids per bidder per item

  // records the block id on which this auction was born and will conclude
  BlockIdType start_block_ = std::numeric_limits<std::size_t>::max();
  BlockIdType end_block_   = std::numeric_limits<std::size_t>::max();

  ItemsContainerType items_{};
  ItemBidsType       item_bids_{};
  ItemsContainerType items_winners_{};
  ItemBidCountType       agent_item_bid_count_{};

//  // id of auction initiator
//  AgentIdType initiator_;

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
//  Auction(std::size_t start_block_id, std::size_t end_block_id, std::size_t max_items, std::size_t initiator)
  Auction(std::size_t start_block_id, std::size_t end_block_id, std::size_t max_items)
    : max_items_(max_items)
    , start_block_(start_block_id)
    , end_block_(end_block_id)
//    , initiator_(initiator)
  {
    // checks for auction validity go here.

    // perhaps this should be a runtime error?
    assert(end_block_ > start_block_);

    auction_valid_ = true;
  }

  std::vector<ItemIdType> ShowListedItems()
  {
    std::vector<ItemIdType> ret_vec{};
    for (auto & it : items_)
    {
      ret_vec.push_back(it.first);
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
  ErrorCode AddItem(ItemIdType const &item_id, AgentIdType const &seller_id, ValueType const &min_price)
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
        items_.insert(std::pair<ItemIdType, AgentIdType>(item_id, seller_id));
        item_bids_[item_id] = BidsContainerType{};
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
          item_bids_[item_id].insert(std::pair<AgentIdType, ValueType>(bidder, bid));
        }
        else
        {
          item_bids_[item_id][bidder] = bid;
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

  void Execute(BlockIdType current_block)
  {
    if((end_block_ == current_block) && auction_valid_)
    {

      // pick winning bids

      // for every item in the auction
      for (auto &cur_item_it : item_bids_)
      {
        BidsContainerType &cur_item_all_bids = cur_item_it.second;

        // find highest bid for this item and set winner
        ValueType   cur_max_bid = std::numeric_limits<ValueType>::min();
        AgentIdType cur_winner  = std::numeric_limits<AgentIdType>::min();
        for (auto &cur_bid_it : cur_item_all_bids)
        {
          if (cur_bid_it.second > cur_max_bid)
          {
            cur_max_bid = cur_bid_it.second;
            cur_winner  = cur_bid_it.first;
          }
        }

        items_winners_.insert(std::pair<ItemIdType, AgentIdType>(cur_item_it.first, cur_winner));
      }

      // deduct funds from winner

      // transfer item to winner

      // close auction
      auction_valid_ = false;
    }
  }

  ItemsContainerType Winners()
  {
    return items_winners_;
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
   * @param bidder
   * @param item_id
   * @return
   */
  std::size_t GetBidsCount(AgentIdType bidder, ItemIdType item_id)
  {
    BidCountType::iterator item_bid_it = agent_item_bid_count_[bidder].find(item_id);
    if (item_bid_it == agent_item_bid_count_[bidder].end())
    {
      return 0;
    }
    return agent_item_bid_count_[bidder][item_id];
  }

  /**
   * update number of bids
   * @param bidder
   * @param item_id
   */
  void IncrementBidCount(AgentIdType bidder, ItemIdType item_id)
  {
    BidCountType::iterator item_bid_it = agent_item_bid_count_[bidder].find(item_id);
    if (item_bid_it == agent_item_bid_count_[bidder].end())
    {
      agent_item_bid_count_[bidder].insert(std::pair<ItemIdType, std::size_t>(item_id, 1));
    }
    else
    {
      ++agent_item_bid_count_[bidder][item_id];
    }
  }
};

} // auctions
} // fetch

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