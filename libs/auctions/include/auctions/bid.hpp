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
  Bid(BidId id, std::vector<Item> items, Value price, AgentId bidder,
      std::vector<Bid> excludes = {})
    : id(id)
    , price(price)
    , bidder(bidder)
    , excludes(std::move(excludes))
    , items_(std::move(items))
  {
    assert(items_.size() > 0);
  }

  std::vector<Item> items() const
  {
    return items_;
  }

  BidId            id     = DEFAULT_BID_ID;
  Value            price  = DEFAULT_BID_PRICE;
  AgentId          bidder = DEFAULT_BID_BIDDER;
  std::vector<Bid> excludes{};

private:
  std::vector<Item> items_{};
};

}  // namespace auctions
}  // namespace fetch