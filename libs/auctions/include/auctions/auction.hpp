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
  enum class AuctionState
  {
    INITIALISED,
    LISTING,
    MINING,
    CLEARED
  };

protected:
  // Auction parameters

  bool        smart_market_      = false;  // is it a smart market
  std::size_t max_items_         = 1;      // max items in auction
  std::size_t max_bids_          = 1;      // max bids per bidder per item
  std::size_t max_bids_per_item_ = std::numeric_limits<std::size_t>::max();  //
  std::size_t max_items_per_bid_ = 1;                                        //

  ItemContainer                     items_{};
  std::vector<fetch::auctions::Bid> bids_{};

  // a valid auction is ongoing (i.e. neither concluded nor yet to begin)
  AuctionState auction_valid_ = AuctionState::INITIALISED;

public:
  /**
   * constructor for an auction
   * @param start_block_id  defines the start time of an auction
   * @param item  defines the item to be sold
   * @param initiator  the id of the agent initiating the auction
   */
  explicit Auction(bool        smart_market = false,
                   std::size_t max_bids     = std::numeric_limits<std::size_t>::max())
    : smart_market_(smart_market)
    , max_bids_(max_bids)
  {
    if (smart_market)
    {
      max_items_         = std::numeric_limits<std::size_t>::max();
      max_bids_per_item_ = std::numeric_limits<std::size_t>::max();
      max_items_per_bid_ = std::numeric_limits<std::size_t>::max();
    }

    // must be some items in the auction!
    assert(max_items_ > 0);

    auction_valid_ = AuctionState::LISTING;
  }
  virtual ~Auction() = default;

  ErrorCode         AddItem(Item const &item);
  ErrorCode         PlaceBid(Bid bid);
  ErrorCode         ShowAuctionResult();
  virtual ErrorCode Execute() = 0;
  ErrorCode         Reset();

  AgentId              Winner(ItemId item_id);
  std::vector<AgentId> Winners();
  ItemContainer        items();
  std::vector<Item>    ShowListedItems() const;
  std::vector<Bid>     ShowBids() const;

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
