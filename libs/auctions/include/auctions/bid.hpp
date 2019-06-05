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

class Item;

constexpr BidId   DEFAULT_BID_ID     = std::numeric_limits<BidId>::max();
constexpr Value   DEFAULT_BID_PRICE  = std::numeric_limits<Value>::max();
constexpr AgentId DEFAULT_BID_BIDDER = std::numeric_limits<AgentId>::max();

/**
 * A bid upon (potentially many) items
 */
class Bid
{
public:
  Bid(BidId id, std::vector<ItemId> item_ids, Value price, AgentId bidder)
    : id(id)
    , price(price)
    , bidder(bidder)
    , item_ids_(std::move(item_ids))
  {
    assert(item_ids_.size() > 0);
    exclude_all = false;
  }

  Bid(BidId id, std::vector<ItemId> item_ids, Value price, AgentId bidder,
      std::vector<BidId> excludes)
    : id(id)
    , price(price)
    , bidder(bidder)
    , excludes(std::move(excludes))
    , item_ids_(std::move(item_ids))
  {
    assert(item_ids_.size() > 0);
    exclude_all = false;
  }

  Bid(BidId id, std::vector<ItemId> item_ids, Value price, AgentId bidder, bool exclude_all)
    : id(id)
    , price(price)
    , bidder(bidder)
    , exclude_all(exclude_all)
    , item_ids_(std::move(item_ids))
  {
    assert(item_ids_.size() > 0);
  }

  std::vector<ItemId> item_ids() const
  {
    return item_ids_;
  }

  BidId              id     = DEFAULT_BID_ID;
  Value              price  = DEFAULT_BID_PRICE;
  AgentId            bidder = DEFAULT_BID_BIDDER;
  std::vector<BidId> excludes{};
  bool               exclude_all = false;

private:
  std::vector<ItemId> item_ids_{};
};

}  // namespace auctions
}  // namespace fetch
