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
  ValueType funds = 0;

  Bidder(std::size_t id, std::size_t funds)
    : id(id)
    , funds(funds)
  {}

private:
  Bidder() = default;
};

FirstPriceAuction SetupAuction(std::size_t start_block_val, std::size_t end_block_val)
{
  BlockIdType start_block(start_block_val);
  BlockIdType end_block(end_block_val);
  return FirstPriceAuction(start_block, end_block);
}

TEST(first_price_auction, one_item_many_bid_first_price_auction)
{
  ErrorCode err;

  // set up auction
  std::size_t       start_block = 10000;
  std::size_t       end_block   = 10010;
  FirstPriceAuction a           = SetupAuction(start_block, end_block);

  // add item to auction
  ItemIdType  item_id   = 0;
  AgentIdType seller_id = 999;
  ValueType   min_price = 7;
  Item        item(item_id, seller_id, min_price);
  err = a.AddItem(item);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

  // set up bidders
  std::size_t         n_bidders = 10;
  std::vector<Bidder> bidders{};
  for (std::size_t i = 0; i < n_bidders; ++i)
  {
    bidders.push_back(Bidder(i, i));
  }

  // make bids
  BidIdType bid_id;
  for (std::size_t j = 0; j < n_bidders; ++j)
  {
    bid_id = static_cast<BidIdType>(j);
    Bid cur_bid(bid_id, {item}, bidders[j].funds, bidders[j].id);
    err = a.PlaceBid(cur_bid);
    ASSERT_TRUE(err == ErrorCode::SUCCESS);
  }

  std::size_t cur_block       = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = a.Execute(BlockIdType(cur_block));
    if (success)
    {
      execution_block = cur_block;
    }
    cur_block++;
  }

  ASSERT_TRUE(execution_block == end_block);
  ASSERT_TRUE(a.Winner(item.Id()) == bidders[bidders.size() - 1].id);
  ASSERT_TRUE(a.Items()[0].SellPrice() == bidders[bidders.size() - 1].funds);
}
