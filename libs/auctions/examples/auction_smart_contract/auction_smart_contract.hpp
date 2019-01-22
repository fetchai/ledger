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

#include "auction_smart_contract/mock_ledger.hpp"
#include "auctions/type_def.hpp"
#include "auctions/vickrey_auction.hpp"

namespace fetch {
namespace auctions {
namespace example {

// 0. set up smart contract
// 1. add items phase (bids may also be permitted during this phase)
// 2. add bids phase (hashed - bids require a small deposit)
// 3. reveal bids phase (bidders must submit new transaction to reveal bid - bidders may be
// penalized half their deposit if they do not)
// 4. collection phase (biiders submit transaction to collect with necessary funds - bidders may be
// penalized half their deposit if winning bidders do not) 4b. (optional) multiple rolling
// collection phases (2nd winner, 3rd winner) back to phase 1.
enum class AuctionPhase
{
  LISTING,
  BIDDING,
  REVEAL,
  COLLECTION
};

enum class Mode
{
  ADD_ITEMS,
  PLACE_BIDS,
  REVEAL_BIDS,
  COLLECT_WINNINGS
};

class VickreyAuctionContract
{
public:
  VickreyAuctionContract(AgentId contract_owner_id, BlockId end_block,
                         std::shared_ptr<MockLedger> ledger);
  ~VickreyAuctionContract() = default;

  bool Call(AgentId           my_id,           // caller id
            Mode              mode,            // what is this transaction for?
            std::vector<Item> all_items = {},  // optional arguments depending on mode
            std::vector<Bid>  all_bids  = {},  // optional arguments depending on mode
            Value             winning_funds =
                std::numeric_limits<Value>::max())  // optoinal argument depending on mode
  {

    AuctionPhase ap = DeterminePhase();

    switch (mode)
    {
    case Mode::ADD_ITEMS:
    {
      if (ap == AuctionPhase::LISTING)
      {
        for (std::size_t j = 0; j < all_items.size(); ++j)
        {
          va_.AddItem(all_items[j]);
        }
        // TODO - LOCK ITEM OWNERSHIP / ESCROW
        return true;
      }
      return false;
    }
    case Mode::PLACE_BIDS:
    {
      if ((ap == AuctionPhase::BIDDING) || (ap == AuctionPhase::LISTING))
      {
        for (std::size_t j = 0; j < all_bids.size(); ++j)
        {
          va_.PlaceBid(all_bids[j]);
        }
        return true;
      }
      return false;
    }
    case Mode::REVEAL_BIDS:
    {
      if (ap == AuctionPhase::REVEAL)
      {
        // TODO - REVEAL HASHED BIDS
        return true;
      }
      return false;
    }
    case Mode::COLLECT_WINNINGS:
    {
      if (ap == AuctionPhase::COLLECTION)
      {
        for (std::size_t j = 0; j < va_.items().size(); ++j)
        {
          if (va_.items()[j].winner == my_id)
          {
            if (winning_funds >= va_.items()[j].sell_price)
            {
              // tell back end library to make checks and then transfer funds and ownership of item
              // TODO - winner assignment not yet handled (interface between auction and ledger
              // needed)
              //              va_.AssignWinner(my_id, winning_funds, va_.items()[j].id);
            }
          }
        }

        // refund any remaining funds to winner
        // AddFunds(my_id, winning_funds);
        return true;
      }
      return false;
    }
    }
  }

private:
  AuctionPhase DeterminePhase()
  {
    std::size_t ap_state = ledger_ptr_->CurBlockNum() % 40;
    if (ap_state < 10)
    {
      return AuctionPhase::LISTING;
    }
    else if (ap_state < 20)
    {
      return AuctionPhase::BIDDING;
    }
    else if (ap_state < 30)
    {
      return AuctionPhase::REVEAL;
    }
    else
    {
      return AuctionPhase::COLLECTION;
    }
  }

  std::vector<Item>    ShowListedItems() const;
  std::vector<Bid>     ShowBids() const;
  ErrorCode            AddItem(Item const &item);
  ErrorCode            PlaceBid(Bid bid);
  AgentId              Winner(ItemId item_id);
  std::vector<AgentId> Winners();
  ItemContainer        items();

  AgentId                     contract_owner_id_ = std::numeric_limits<AgentId>::max();
  VickreyAuction              va_;
  std::shared_ptr<MockLedger> ledger_ptr_ = nullptr;
};

}  // namespace example
}  // namespace auctions
}  // namespace fetch
