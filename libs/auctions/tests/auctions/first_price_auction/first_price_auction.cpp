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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include <auctions/first_price_auction.hpp>

using namespace fetch::auctions;

class Bidder
{
public:
  std::size_t id    = 0;
  Value       funds = 0;

  Bidder(std::size_t id, Value funds)
    : id(id)
    , funds(funds)
  {}

private:
  Bidder() = default;
};

TEST(first_price_auction, one_item_many_bid_first_price_auction)
{
  ErrorCode err;

  // set up auction
  FirstPriceAuction a = FirstPriceAuction();

  // add item to auction
  ItemId  item_id   = 0;
  AgentId seller_id = 999;
  Value   min_price = 7;
  Item    item(item_id, seller_id, min_price);
  err = a.AddItem(item);
  ASSERT_EQ(err, ErrorCode::SUCCESS);

  // set up bidders
  std::size_t         n_bidders = 10;
  std::vector<Bidder> bidders{};
  for (std::size_t i = 0; i < n_bidders; ++i)
  {
    bidders.push_back(Bidder(i, static_cast<Value>(i)));
  }

  // make bids
  BidId bid_id;
  for (std::size_t j = 0; j < n_bidders; ++j)
  {
    bid_id = static_cast<BidId>(j);
    Bid cur_bid(bid_id, {item.id}, bidders[j].funds, bidders[j].id);
    err = a.PlaceBid(cur_bid);
    ASSERT_EQ(err, ErrorCode::SUCCESS);
  }

  err = a.Execute();
  ASSERT_EQ(err, ErrorCode::SUCCESS);

  ASSERT_EQ(a.Winner(item.id), bidders[bidders.size() - 1].id);
  ASSERT_EQ(a.items()[0].sell_price, bidders[bidders.size() - 1].funds);
}
