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

#include "auctions/type_def.hpp"

namespace fetch {
namespace auctions {

constexpr ItemIdType  DefaultItemId        = std::numeric_limits<ItemIdType>::max();
constexpr AgentIdType DefaultItemAgentId   = std::numeric_limits<AgentIdType>::max();
constexpr ValueType   DefaultItemMinPrice  = std::numeric_limits<AgentIdType>::max();
constexpr ValueType   DefaultItemMaxBid    = std::numeric_limits<ValueType>::min();
constexpr ValueType   DefaultItemSellPrice = std::numeric_limits<ValueType>::min();

constexpr AgentIdType DefaultItemWinner = std::numeric_limits<AgentIdType>::max();

/**
 * An item in the auction which may be bid upon
 */
class Item
{
public:
  Item() = default;
  Item(ItemIdType id, AgentIdType seller_id, ValueType min_price)
    : id_(id)
    , seller_id_(seller_id)
    , min_price_(min_price)
  {}

  /**
   * ID accessor
   * @return
   */
  ItemIdType Id() const
  {
    return id_;
  }
  ItemIdType &Id()
  {
    return id_;
  }

  /**
   * ID setter
   */
  void Id(ItemIdType &id)
  {
    id_ = id;
  }

  /**
   * seller_id accessor
   * @return
   */
  AgentIdType SellerId() const
  {
    return id_;
  }
  AgentIdType &SellerId()
  {
    return id_;
  }

  /**
   * seller_id setter
   */
  void SellerId(AgentIdType &seller_id)
  {
    seller_id_ = seller_id;
  }

  /**
   * min_price accessor
   * @return
   */
  ValueType MinPrice() const
  {
    return min_price_;
  }
  ValueType &MinPrice()
  {
    return min_price_;
  }

  /**
   * min_price setter
   */
  void MinPrice(ValueType &min_price)
  {
    min_price_ = min_price;
  }

  /**
   * MaxBid accessor
   * @return
   */
  ValueType MaxBid() const
  {
    return max_bid_;
  }
  ValueType &MaxBid()
  {
    return max_bid_;
  }

  /**
   * MaxBid setter
   */
  void MaxBid(ValueType &max_bid)
  {
    max_bid_ = max_bid;
  }
  /**
   * SellPrice accessor
   * @return
   */
  ValueType SellPrice() const
  {
    return sell_price_;
  }
  ValueType &SellPrice()
  {
    return sell_price_;
  }

  /**
   * SellPrice setter
   */
  void SellPrice(ValueType &sell_price)
  {
    sell_price_ = sell_price;
  }
  /**
   * bids accessor
   * @return
   */
  std::vector<Bid> Bids() const
  {
    return bids_;
  }
  std::vector<Bid> &Bids()
  {
    return bids_;
  }

  /**
   * bids setter
   */
  void Bids(std::vector<Bid> &bids)
  {
    bids_ = bids;
  }

  /**
   * BidCount accessor
   * @return
   */
  std::uint32_t BidCount() const
  {
    return bid_count_;
  }
  std::uint32_t &BidCount()
  {
    return bid_count_;
  }

  /**
   * BidCount setter
   */
  void BidCount(std::uint32_t &bid_count)
  {
    bid_count_ = bid_count;
  }
  /**
   * winner accessor
   * @return
   */
  AgentIdType Winner() const
  {
    return winner_;
  }
  AgentIdType &Winner()
  {
    return winner_;
  }

  /**
   * winner setter
   */
  void Winner(AgentIdType &winner)
  {
    winner_ = winner;
  }
  /**
   * ID accessor
   * @return
   */
  std::unordered_map<AgentIdType, std::size_t> AgentBidCount() const
  {
    return agent_bid_count_;
  }
  std::unordered_map<AgentIdType, std::size_t> &AgentBidCount()
  {
    return agent_bid_count_;
  }

  /**
   * ID setter
   */
  void AgentBidCount(std::unordered_map<AgentIdType, std::size_t> &agent_bid_count)
  {
    agent_bid_count_ = agent_bid_count;
  }

private:
  ItemIdType  id_        = DefaultItemId;
  AgentIdType seller_id_ = DefaultItemAgentId;
  ValueType   min_price_ = DefaultItemMinPrice;

  ValueType max_bid_    = DefaultItemMaxBid;
  ValueType sell_price_ = DefaultItemSellPrice;

  std::vector<Bid> bids_{};

  std::uint32_t bid_count_ = 0;
  AgentIdType   winner_    = DefaultItemWinner;

  //  std::unordered_map<AgentIdType, ValueType>   bids{};
  std::unordered_map<AgentIdType, std::size_t> agent_bid_count_{};
};

}  // namespace auctions
}  // namespace fetch