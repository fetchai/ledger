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

#include "auctions/bid.hpp"
#include "auctions/error_codes.hpp"
#include "auctions/item.hpp"
#include "auctions/type_def.hpp"

namespace fetch {
namespace auctions {

constexpr std::size_t DEFAULT_SIZE_T_BLOCK_ID = std::numeric_limits<std::size_t>::max();

// template <typename A, typename V = fetch::ml::Variable<A>>
class Auction
{

public:
protected:
  // Auction parameters

  bool        smart_market_      = false;  // is it a smart market
  std::size_t max_items_         = 1;      // max items in auction
  std::size_t max_bids_          = 1;      // max bids per bidder per item
  std::size_t max_bids_per_item_ = std::numeric_limits<std::size_t>::max();  //
  std::size_t max_items_per_bid_ = 1;                                        //

  // records the block id on which this auction will conclude
  BlockId end_block_ = std::numeric_limits<BlockId>::max();

  ItemContainer                     items_{};
  std::vector<fetch::auctions::Bid> bids_{};

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
  explicit Auction(BlockId     end_block_id = BlockId(DEFAULT_SIZE_T_BLOCK_ID),
                   bool        smart_market = false,
                   std::size_t max_bids     = std::numeric_limits<std::size_t>::max())
    : smart_market_(smart_market)
    , max_bids_(max_bids)
    , end_block_(std::move(end_block_id))
  {
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

  std::vector<Item>    ShowListedItems() const;
  std::vector<Bid>     ShowBids() const;
  ErrorCode            AddItem(Item const &item);
  ErrorCode            PlaceBid(Bid bid);
  AgentId              Winner(ItemId item_id);
  std::vector<AgentId> Winners();
  ItemContainer        items();

  /**
   * Executes the auction by identifying the winners, and making appropriate transfers
   * @param current_block
   * @return
   */
  virtual bool Execute(BlockId current_block) = 0;

private:
  bool         ItemInAuction(ItemId const &item_id) const;
  std::size_t  GetBidsCount(AgentId const &bidder, ItemId const &item_id) const;
  std::size_t  GetBidsCount(ItemId item_id) const;
  void         IncrementBidCount(AgentId bidder, ItemId item_id);
  virtual void SelectWinners() = 0;
  ErrorCode    CheckItemValidity(Item const &item) const;
  ErrorCode    CheckBidValidity(Bid const &bid) const;
};

}  // namespace auctions
}  // namespace fetch
