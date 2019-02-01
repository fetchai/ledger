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

class Bid;

constexpr ItemId  DEFAULT_ITEM_ID         = std::numeric_limits<ItemId>::max();
constexpr AgentId DEFAULT_ITEM_AGENT_ID   = std::numeric_limits<AgentId>::max();
constexpr Value   DEFAULT_ITEM_MIN_PRICE  = std::numeric_limits<Value>::max();
constexpr Value   DEFAULT_ITEM_MAX_BID    = std::numeric_limits<Value>::min();
constexpr Value   DEFAULT_ITEM_SELL_PRICE = std::numeric_limits<Value>::min();
constexpr AgentId DEFAULT_ITEM_WINNER     = std::numeric_limits<AgentId>::max();

/**
 * An item in the auction which may be bid upon
 */
class Item
{
public:
  Item() = default;
  Item(ItemId id, AgentId seller_id, Value min_price)
    : id(id)
    , seller_id(seller_id)
    , min_price(min_price)
  {}

  ItemId  id        = DEFAULT_ITEM_ID;
  AgentId seller_id = DEFAULT_ITEM_AGENT_ID;
  Value   min_price = DEFAULT_ITEM_MIN_PRICE;

  Value max_bid    = DEFAULT_ITEM_MAX_BID;
  Value sell_price = DEFAULT_ITEM_SELL_PRICE;

  std::vector<Bid> bids{};

  std::uint32_t bid_count = 0;
  BidId         winner    = DEFAULT_ITEM_WINNER;

  std::unordered_map<AgentId, std::size_t> agent_bid_count{};
};

}  // namespace auctions
}  // namespace fetch