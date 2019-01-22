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
#include "auctions/vickrey_auction.hpp"

#include <atomic>

namespace fetch {
namespace auctions {

enum class MODE
{
  PLACE_BID,
  ADD_ITEM
};

class VickreyAuctionContract
{
public:

//  VickreyAuctionContract();
  VickreyAuctionContract(AgentId contract_owner_id, BlockId start_block, BlockId end_block);
//    {
//    };

  ~VickreyAuctionContract() = default;

  static constexpr char const *LOGGING_NAME = "DummyContract";

  std::size_t counter() const
  {
    return counter_;
  }


  // 0. set up smart contract
  // 1. add items phase (bids may also be permitted during this phase)
  // 2. add bids phase (hashed - bids require a small deposit)
  // 3. reveal bids phase (bidders must submit new transaction to reveal bid - bidders may be penalized half their deposit if they do not)
  // 4. collection phase (biiders submit transaction to collect with necessary funds - bidders may be penalized half their deposit if winning bidders do not)
  // 4b. (optional) multiple rolling collection phases (2nd winner, 3rd winner)
  // back to phase 1.


  void Call(AgentId my_id, MODE mode, Value price, std::vector<Item> all_items)
  {
    // check my_id is true


    switch (mode)
    {
      case MODE::PLACE_BID:
      {
        Bid(GenerateNewBidId(), all_items, price, my_id)
        va.PlaceBid()

      }
      case MODE::ADD_ITEM:
      {

        //


        // lock item ownership

      }
      case MODE::EXECUTE:
      {

        if (some_block_condition && (my_id == contract_owner_id));
        status = va.Execute();

        ChangeFunds(status.id);
        AddItemOwnership(status.id);



      }
    }
  }

private:

    std::vector<Item>    ShowListedItems() const;
  std::vector<Bid>     ShowBids() const;
  ErrorCode            AddItem(Item const &item);
  ErrorCode            PlaceBid(Bid bid);
  AgentId              Winner(ItemId item_id);
  std::vector<AgentId> Winners();
  ItemContainer        items();






  using Counter = std::atomic<std::size_t>;

  VickreyAuction va;

  Counter counter_{0};
};

}  // namespace ledger
}  // namespace fetch
