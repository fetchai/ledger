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
/**
 * An item in the auction which may be bid upon
 */
class Item
{

  ItemIdType  id         = 0;
  AgentIdType seller_id  = 0;
  ValueType   min_price  = std::numeric_limits<ValueType>::max();
  ValueType   max_bid    = std::numeric_limits<ValueType>::min();
  ValueType   sell_price = std::numeric_limits<ValueType>::min();

  std::vector<Bid> bids{};

  std::size_t bid_count = 0;
  AgentIdType winner    = 0;

  //  std::unordered_map<AgentIdType, ValueType>   bids{};
  std::unordered_map<AgentIdType, std::size_t> agent_bid_count{};
};

} // auctions
} // fetch