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

enum class ErrorCodes
{
    Success;
    ItemAlreadyListed;
    AuctionFull;
    ItemNotListed;
    TooManyBids;

};

//template <typename A, typename V = fetch::ml::Variable<A>>
class Auction
{
    using ErrorCode = typename ErrorCodes;

    using ValueType = typename std::size_t;

    using AgentIdType = typename std::size_t;
    using ItemIdType = typename std::size_t;

    using ItemsContainerType = typename std::unordered_map<ItemIdType, AgentIdType>;

    // contains multiple agents' bids
    using BidsContainerType = typename std::unordered_map<AgentIdType, ValueType>;

    // contains information on the item id, the owner id, and all bids
    using ItemBidsType = typename std::unordered_map<ItemIdType, BidsContainerType>;

    using BidCountType = typename std::unordered_map<ItemIdType, std::unordered_map<AgentIdType, std::size_t>>;


private:

    // Auction parameters

    std::size_t max_items_ = 0;   // max items in auction
    std::size_t max_bids_ = 1;    // max bids per bidder per item

    // records the block id on which this auction was born and will conclude
    std::size_t start_block_ = std::numeric_limits<std::size_t>::max();
    std::size_t end_block_ = std::numeric_limits<std::size_t>::max();

    ItemsContainerType items_{};
    ItemBidsType item_bids_{};
    BidCountType agent_item_bid_count_{};

    // id of auction initiator
    AgentType initiator_;

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
    Auction(std::size_t start_block_id,
            std::size_t end_block_id,
            std::size_t item_description,
            std::size_t max_items,
            std::size_t initiator) : start_block_(start_block_id),
            end_block_(end_block_id),
            item_description_(item_description),
            max_items_(max_items),
            initiator_(initiator)
    {
      // checks for auction validity go here.

      // perhaps this should be a runtime error?
      assert(end_block_id > start_block_id);

      auction_valid_ = true;
    }

    ItemType ShowItems()
    {
    }

    /**
     * adds an item to the auction
     * @param item
     * @param item_owner
     * @param min_price
     * @return
     */
    ErrorCode AddItem(ItemType const &item, ValueType const &min_price)
    {

      // check if auction full
      if (items_.size() >= max_items_)
      {
        return ErrorCodes::AuctionFull;
      }

      // auction not full
      else
      {
        // check if item already listed
        if (std::find(vector.begin(), vector.end(), items_[item]) != vector.end())
        {
          return ErrorCodes::ItemAlreadyListed; // item already listed error

          // TODO () - update trustworthiness of agent that try to double list items
        }

        // list the items
        else
        {
          items_ = item;
          items_bids_[item] = BidsContainerType{};
          return ErrorCodes::Success; // success
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
        return ErrorCodes::ItemNotListed;
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

          return ErrorCodes::Success;

        }
        else
        {
          return ErrorCodes::TooManyBids;
        }

      }
    }


    void ExecuteAuction()
    {

    }

private:

    /**
     * checks whether item in the auction
     * @param item_id
     * @return
     */
    bool ItemInAuction(ItemIdType item_id)
    {
      ItemType::iterator item_it = items_.find(item_id);
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
        ItemBidsType::iterator item_bid_it = agent_item_bid_count_[item_id].find(bidder);
        if (item_bid_it == agent_item_bid_count_[item_id].end())
        {
          return 0;
        }
        return agent_item_bid_count_[item_id][bidder];
    }

    /**
     * update number of bids
     * @param bidder
     * @param item_id
     */
    void IncrementBidCount(AgentIdType bidder, ItemIdType item_id)
    {
      ItemBidsType::iterator item_bid_it = agent_item_bid_count_[item_id].find(bidder);
        if (item_bid_it == agent_item_bid_count_[item_id].end())
        {
          agent_item_bid_count_[item_id].insert(std::pair<AgentIdType, std::size_t>(bidder, 1));
        }
        else
        {
          ++agent_item_bid_count_[item_id][bidder];
        }
    }



}


//
//    def BuildGraph(self):
//        # Building graph
//        self.item_winner = [(-1, -1) for i in range(len(self.items))]
//        self.active_bid = [ [False for j in range(len(self.bids[i]))] for i in range(len(self.bids))]
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