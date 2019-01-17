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

constexpr BidIdType   DefaultBidId     = std::numeric_limits<BidIdType>::max();
constexpr ValueType   DefaultBidPrice  = std::numeric_limits<ValueType>::max();
constexpr AgentIdType DefaultBidBidder = std::numeric_limits<AgentIdType>::max();

/**
 * A bid upon (potentially many) items
 */
class Bid
{
public:
  Bid(BidIdType id, std::vector<Item> items, ValueType price, AgentIdType bidder,
      std::vector<Bid> excludes = {})
    : id_(id)
    , items_(std::move(items))
    , price_(price)
    , bidder_(bidder)
    , excludes_(std::move(excludes))
  {
    assert(items_.size() > 0);
  }

  /**
   * ID accessor
   * @return
   */
  BidIdType Id() const
  {
    return id_;
  }
  BidIdType &Id()
  {
    return id_;
  }

  /**
   * ID setter
   */
  void Id(BidIdType &id)
  {
    id_ = id;
  }

  /**
   * items accessor
   * @return
   */
  std::vector<Item> Items() const
  {
    return items_;
  }
  std::vector<Item> &Items()
  {
    return items_;
  }

  /**
   * Items setter
   * @param items
   */
  void Items(std::vector<Item> const &items)
  {
    items_ = items;
  }

  /**
   * price accessor
   * @return
   */
  ValueType Price() const
  {
    return price_;
  }
  ValueType &Price()
  {
    return price_;
  }

  /**
   * price setter
   * @param price
   */
  void Price(ValueType price)
  {
    price_ = price;
  }

  /**
   * bidder accessor
   * @return
   */
  AgentIdType Bidder() const
  {
    return bidder_;
  }
  AgentIdType &Bidder()
  {
    return bidder_;
  }

  /**
   * bidder setter
   * @param price
   */
  void Bidder(AgentIdType bidder)
  {
    bidder_ = bidder;
  }

  /**
   * excludes accessor
   * @return
   */
  std::vector<Bid> Excludes() const
  {
    return excludes_;
  }
  std::vector<Bid> &Excludes()
  {
    return excludes_;
  }

  /**
   * excludes setter
   * @param price
   */
  void Excludes(std::vector<Bid> excludes)
  {
    excludes_ = excludes;
  }

private:
  BidIdType         id_ = DefaultBidId;
  std::vector<Item> items_{};
  ValueType         price_  = DefaultBidPrice;
  AgentIdType       bidder_ = DefaultBidBidder;
  std::vector<Bid>  excludes_{};
};

}  // namespace auctions
}  // namespace fetch