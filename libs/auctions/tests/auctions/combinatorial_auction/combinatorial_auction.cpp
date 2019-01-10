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

#include <auctions/combinatorial_auction.hpp>

using namespace fetch::auctions;

class Bidder
{
public:
  std::size_t id    = 0;
  std::size_t funds = 0;

  Bidder(std::size_t id, std::size_t funds)
    : id(id)
    , funds(funds)
  {}

private:
  Bidder() = default;
};

CombinatorialAuction SetupAuction(std::size_t start_block_val, std::size_t end_block_val)
{
  BlockIdType start_block(start_block_val);
  BlockIdType end_block(end_block_val);
  return CombinatorialAuction(start_block, end_block);
}

TEST(combinatorial_auction, many_bid_many_item_auction)
{
  // set up auction
  std::size_t          start_block = 10000;
  std::size_t          end_block   = 10010;
  CombinatorialAuction ca          = SetupAuction(start_block, end_block);

  // add items to auction
  std::vector<Item> items{};

  Item item1;
  item1.id        = 0;
  item1.seller_id = 990;
  item1.min_price = 10;
  items.push_back(item1);

  Item item2;
  item2.id        = 1;
  item2.seller_id = 990;
  item2.min_price = 15;
  items.push_back(item2);

  Item item3;
  item3.id        = 2;
  item3.seller_id = 990;
  item3.min_price = 25;
  items.push_back(item3);

  Item item4;
  item4.id        = 3;
  item4.seller_id = 990;
  item4.min_price = 2;
  items.push_back(item4);

  for (std::size_t k = 0; k < items.size(); ++k)
  {
    ErrorCode err = ca.AddItem(items[k]);
    ASSERT_TRUE(err == ErrorCode::SUCCESS);
  }

  // set up bidders
  std::vector<Bidder> bidders{};
  bidders.push_back(Bidder(0, 500));

  // make bids
  Bid bid1;
  bid1.id     = 0;
  bid1.price  = 1;
  bid1.bidder = bidders[0].id;
  bid1.items.push_back(items[3]);
  ca.Bid(bid1);

  Bid bid2;
  bid2.id     = 1;
  bid2.price  = 30;
  bid2.bidder = bidders[0].id;
  bid2.items.push_back(items[1]);
  bid2.items.push_back(items[2]);
  bid2.items.push_back(items[3]);
  bid2.excludes.push_back(bid1);
  ca.Bid(bid2);

  Bid bid3;
  bid3.id     = 2;
  bid3.price  = 20;
  bid3.bidder = bidders[0].id;
  bid3.items.push_back(items[0]);
  bid3.items.push_back(items[3]);
  bid3.excludes.push_back(bid2);
  bid3.excludes.push_back(bid1);
  ca.Bid(bid3);

  // Mine the smart market
  for (std::size_t i = 0; i < 100; ++i)
  {
    ca.Mine(i, 1000);
  }

  // execute / terminate the auction
  std::size_t cur_block       = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = ca.Execute(BlockIdType(cur_block));
    if (success)
    {
      execution_block = cur_block;
    }
    cur_block++;
  }

  for (std::size_t j = 0; j < 3; ++j)
  {
    std::cout << "ca.active_[j]: " << ca.Active(j) << std::endl;

    //    ASSERT_TRUE(execution_block == end_block);
    //    ASSERT_TRUE(ca.Winner(j) == bidders[bidders.size() - 1].id);
    //    ASSERT_TRUE(ca.Items()[j].sell_price == bidders[bidders.size() - 2].funds / 10);
    //    ca.Winner(j);
  }
}
