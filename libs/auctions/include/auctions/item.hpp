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
constexpr ValueType   DefaultItemMinPrice  = std::numeric_limits<ValueType>::max();
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

  ItemIdType  id         = DefaultItemId;
  AgentIdType seller_id  = DefaultItemAgentId;
  ValueType   min_price  = DefaultItemMinPrice;
  ValueType   max_bid    = DefaultItemMaxBid;
  ValueType   sell_price = DefaultItemSellPrice;

  std::vector<Bid> bids{};

  std::uint32_t bid_count = 0;
  AgentIdType   winner    = DefaultItemWinner;

  //  std::unordered_map<AgentIdType, ValueType>   bids{};
  std::unordered_map<AgentIdType, std::size_t> agent_bid_count{};
};

}  // namespace auctions
}  // namespace fetch