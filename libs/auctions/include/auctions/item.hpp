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

constexpr ItemId  DefaultItemId        = std::numeric_limits<ItemId>::max();
constexpr AgentId DefaultItemAgentId   = std::numeric_limits<AgentId>::max();
constexpr Value   DefaultItemMinPrice  = std::numeric_limits<Value>::max();
constexpr Value   DefaultItemMaxBid    = std::numeric_limits<Value>::min();
constexpr Value   DefaultItemSellPrice = std::numeric_limits<Value>::min();

constexpr AgentId DefaultItemWinner = std::numeric_limits<AgentId>::max();

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

  ItemId  id        = DefaultItemId;
  AgentId seller_id = DefaultItemAgentId;
  Value   min_price = DefaultItemMinPrice;

  Value max_bid    = DefaultItemMaxBid;
  Value sell_price = DefaultItemSellPrice;

  std::vector<Bid> bids{};

  std::uint32_t bid_count = 0;
  BidId         winner    = DefaultItemWinner;

  std::unordered_map<AgentId, std::size_t> agent_bid_count{};
};

}  // namespace auctions
}  // namespace fetch