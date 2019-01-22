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

#include "type_def.hpp"
#include <limits>

namespace fetch {
namespace auctions {
namespace example {

constexpr AgentId DEFAULT_AGENT_ID = std::numeric_limits<AgentId>::max();

class Agent
{

public:
  bool  items_listed   = false;
  bool  bids_placed    = false;
  bool  bids_revealed  = false;
  bool  wins_collected = false;
  Value funds          = 100;

  Agent(AgentId id)
    : id_(id)
  {
    assert(id_ != DEFAULT_AGENT_ID);
  }

  Agent(AgentId id, std::vector<Item> items)
    : id_(id)
    , items_(std::move(items))
  {
    assert(id_ != DEFAULT_AGENT_ID);
  }

  Agent(AgentId id, std::vector<Bid> bids)
    : id_(id)
    , bids_(std::move(bids))
  {
    assert(id_ != DEFAULT_AGENT_ID);
  }

  AgentId id()
  {
    return id_;
  }

  std::vector<Item> items()
  {
    return items_;
  }

  std::vector<Bid> bids()
  {
    return bids_;
  }

  /**
   * auction contract owner's decision making
   * @param block_num
   * @param cur_phase
   * @return
   */
  AuctionPhase UpdateAuctionPhase(size_t block_num, AuctionPhase cur_phase)
  {
    if (block_num % 10 == 0)
    {
      switch (cur_phase)
      {
      case AuctionPhase::LISTING:
        return AuctionPhase::BIDDING;
      case AuctionPhase::BIDDING:
        return AuctionPhase::REVEAL;
      case AuctionPhase::REVEAL:
        return AuctionPhase::COLLECTION;
      case AuctionPhase::COLLECTION:
        return AuctionPhase::LISTING;
      default:
        return cur_phase;
      }
    }
    else
    {
      return cur_phase;
    }
  }

private:
  AgentId           id_ = DEFAULT_AGENT_ID;
  std::vector<Item> items_{};
  std::vector<Bid>  bids_{};
};

}  // namespace example
}  // namespace auctions
}  // namespace fetch