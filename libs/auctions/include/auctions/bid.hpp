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

constexpr BidIdType DefaultBidId = std::numeric_limits<BidIdType>::max();
constexpr ValueType DefaultBidPrice = std::numeric_limits<ValueType>::max();
constexpr AgentIdType DefaultBidBidder = std::numeric_limits<AgentIdType>::max();

/**
 * A bid upon (potentially many) items
 */
class Bid
{
public:

  Bid() = default;

  BidIdType         id = DefaultBidId;
  std::vector<Item> items{};
  ValueType         price = DefaultBidPrice;
  std::vector<Bid>  excludes{};
  AgentIdType       bidder = DefaultBidBidder;
};

} // auctions
} // fetch