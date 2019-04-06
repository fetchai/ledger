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

/**
 * adds an item to the auction
 * @param item
 * @param item_owner
 * @param min_price
 * @return
 */
ErrorCode Auction::AddItem(Item const &item)
{
  ErrorCode ec = CheckItemValidity(item);
  if (ec == ErrorCode::SUCCESS)
  {
    items_.insert(std::pair<ItemId, Item>(item.id, item));
  }
  return ec;
}

/**
 * returns every listed item
 * @return
 */
std::vector<Item> Auction::ShowListedItems() const
{
  std::vector<Item> ret_vec{};
  for (auto &it : items_)
  {
    ret_vec.push_back(it.second);
  }
  return ret_vec;
}

/**
 * returns every listed item
 * @return
 */
std::vector<Bid> Auction::ShowBids() const
{
  std::vector<Bid> ret_vec{};
  for (auto &it : bids_)
  {
    ret_vec.push_back(it);
  }
  return ret_vec;
}

/**
 * Agent adds a bid (potentially on multiple items)
 * @param bid  a Bid object describing the items to bid on, the price, and the excluded items
 * @return
 */
ErrorCode Auction::PlaceBid(Bid bid)
{
  ErrorCode ec = CheckBidValidity(bid);
  if (ec == ErrorCode::SUCCESS)
  {
    // place bids and update counter
    bids_.push_back(bid);
    for (std::size_t j = 0; j < bid.item_ids().size(); ++j)
    {
      items_[bid.item_ids()[j]].bids.push_back(bid);

      // count of how many times this bidder has bid on this item
      IncrementBidCount(bid.bidder, bid.item_ids()[j]);
    }
  }
  return ec;
}

/**
 * returns winner of auction for a particular item
 * @param item_id
 * @return
 */
AgentId Auction::Winner(ItemId item_id)
{
  return items_[item_id].winner;
}

/**
 * Returns all items in auction
 * @return
 */
ItemContainer Auction::items()
{
  return items_;
}

//////// private ///////

/**
 * checks whether item in the auction
 * @param item_id
 * @return
 */
bool Auction::ItemInAuction(ItemId const &item_id) const
{
  return (items_.find(item_id) != items_.end());
}

/**
 * gets the number of bids this bidder has previous placed on this item in this auction
 * if the item is not in the auction, returns 0
 * @param bidder
 * @param item_id
 * @return
 */
std::size_t Auction::GetBidsCount(AgentId const &bidder, ItemId const &item_id) const
{
  if (ItemInAuction(item_id))
  {
    if (items_.at(item_id).agent_bid_count.find(bidder) == items_.at(item_id).agent_bid_count.end())
    {
      return 0;
    }
    return items_.at(item_id).agent_bid_count.at(bidder);
  }
  return 0;
}

/**
 * gets the number of bids on this item in this auction
 * @param bidder
 * @param item_id
 * @return
 */
std::size_t Auction::GetBidsCount(ItemId item_id) const
{
  if (ItemInAuction(item_id))
  {
    return items_.at(item_id).bid_count;
  }
  return 0;
}

/**
 * update number of bids
 * @param bidder
 * @param item_id
 */
void Auction::IncrementBidCount(AgentId bidder, ItemId item_id)
{
  assert(ItemInAuction(item_id));

  items_[item_id].bid_count++;

  if (items_[item_id].agent_bid_count.find(bidder) == items_[item_id].agent_bid_count.end())
  {
    items_[item_id].agent_bid_count.insert(std::pair<AgentId, std::size_t>(bidder, 1));
  }
  else
  {
    ++items_[item_id].agent_bid_count[bidder];
  }
}

/**
 * series of validity checks for AddItem call
 */
ErrorCode Auction::CheckItemValidity(Item const &item) const
{
  // Item must have a valid ID
  if (item.id == DEFAULT_ITEM_ID)
  {
    return ErrorCode::ITEM_ID_ERROR;
  }

  // Item seller must have a valid ID
  if (item.seller_id == DEFAULT_ITEM_AGENT_ID)
  {
    return ErrorCode::AGENT_ID_ERROR;
  }

  // Item must have a valid minimum price
  if (item.min_price == DEFAULT_ITEM_MIN_PRICE)
  {
    return ErrorCode::ITEM_MIN_PRICE_ERROR;
  }

  // auction must be still open to adding new items
  if (!(auction_valid_ == AuctionState::LISTING))
  {
    return ErrorCode::AUCTION_CLOSED;
  }

  // auction must not be full
  if (items_.size() >= max_items_)
  {
    return ErrorCode::AUCTION_FULL;
  }

  // auction must not have already listed item
  if (items_.find(item.id) != items_.end())
  {
    return ErrorCode::ITEM_ALREADY_LISTED;  // item already listed error
  }

  return ErrorCode::SUCCESS;
}

/**
 * series of validity vhecks for PlaceBid call
 */
ErrorCode Auction::CheckBidValidity(Bid const &bid) const
{
  if (bid.id == DEFAULT_BID_ID)
  {
    return ErrorCode::INVALID_BID_ID;
  }

  for (std::size_t j = 0; j < bids_.size(); ++j)
  {
    if (bids_[j].id == bid.id)
    {
      return ErrorCode::REPEAT_BID_ID;
    }
  }

  if (bid.price == DEFAULT_BID_PRICE)
  {
    return ErrorCode::BID_PRICE;
  }

  if (bid.bidder == DEFAULT_BID_BIDDER)
  {
    return ErrorCode::BID_BIDDER_ID;
  }

  // auction must be still open to adding new bids
  if (!(auction_valid_ == AuctionState::LISTING))
  {
    return ErrorCode::AUCTION_CLOSED;
  }

  // bid must not have more items than permissible
  if (bid.item_ids().size() > max_items_per_bid_)
  {
    return ErrorCode::TOO_MANY_ITEMS;
  }

  for (std::size_t j = 0; j < bid.item_ids().size(); ++j)
  {
    // check item listed in auction
    if (!ItemInAuction(bid.item_ids()[j]))
    {
      return ErrorCode::ITEM_NOT_LISTED;
    }

    // check the bidder has not exceeded their allowed number of bids on this item
    std::size_t n_bids = GetBidsCount(bid.bidder, bid.item_ids()[j]);
    if (n_bids >= max_bids_)
    {
      return ErrorCode::TOO_MANY_BIDS;
    }
  }

  return ErrorCode::SUCCESS;
}

ErrorCode Auction::ShowAuctionResult()
{
  if (!(auction_valid_ == AuctionState::CLEARED))
  {
    return ErrorCode::AUCTION_STILL_LISTING;
  }

  Value total_sales = 0;

  for (auto &item : items())
  {
    std::cout << "item id: " << item.first << std::endl;
    if (item.second.winner == DEFAULT_ITEM_WINNER)
    {
      std::cout << "item unsold" << std::endl;
    }
    else
    {
      std::cout << "winning bid: " << item.second.winner << ", at price: " << item.second.sell_price
                << std::endl;
      total_sales += item.second.sell_price;
    }
    std::cout << std::endl;
  }

  std::cout << "total_sales: " << total_sales << std::endl;
  return ErrorCode::SUCCESS;
}

/**
 * returns all auction winners
 * @return
 */
std::vector<AgentId> Auction::Winners()
{
  std::vector<AgentId> winners{};
  for (auto &item_it : items_)
  {
    winners.push_back(item_it.second.winner);
  }
  return winners;
}

/**
 * reset the auction
 * @return
 */
ErrorCode Auction::Reset()
{
  if (!(auction_valid_ == AuctionState::CLEARED))
  {
    return ErrorCode::PREVIOUS_AUCTION_NOT_CLEARED;
  }

  items_.clear();
  bids_.clear();
  auction_valid_ = AuctionState::INITIALISED;

  return ErrorCode::SUCCESS;
}

}  // namespace auctions
}  // namespace fetch
