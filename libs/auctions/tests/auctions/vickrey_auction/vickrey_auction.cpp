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

#include <auctions/vickrey_auction.hpp>

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

TEST(vickrey_auction, one_bid_auction)
{
  std::size_t seller_id = 999;
  std::size_t start_block = 10000;
  std::size_t end_block = start_block + 9;
  std::size_t max_items = 1;

  // set up auction
  VickreyAuction va(start_block, end_block, max_items);

  // add item to auction
  ItemIdType item      = 0;
  ValueType  min_price = 7;
  va.AddItem(item, seller_id, min_price);

  // set up bidders
  std::vector<Bidder> bidders{};
  bidders.push_back(Bidder(0, 100));

  va.AddSingleBid(bidders[0].funds, bidders[0].id, item);

  std::size_t cur_block = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = va.Execute(cur_block);
    if (success)
    {
      execution_block = cur_block;
    }
    cur_block++;
  }

  std::cout << "Winner ID: " << va.Winner(item) << std::endl;
  std::cout << "Winner bid " << bidders[bidders.size() - 1].id << std::endl;
  std::cout << "Sale price" << va.Items()[0].sell_price << std::endl;

  ASSERT_TRUE(execution_block == end_block);
  ASSERT_TRUE(va.Winner(item) == bidders[0].id);
  ASSERT_TRUE(va.Items()[0].sell_price == bidders[0].funds);
}


TEST(vickrey_auction, two_bid_auction)
{
  std::size_t seller_id = 999;
  std::size_t start_block = 10000;
  std::size_t end_block = start_block + 9;
  std::size_t max_items = 1;

  // set up auction
  VickreyAuction va(start_block, end_block, max_items);

  // add item to auction
  ItemIdType item      = 0;
  ValueType  min_price = 7;
  va.AddItem(item, seller_id, min_price);

  // set up bidders
  std::vector<Bidder> bidders{};
  bidders.push_back(Bidder(0, 100));
  bidders.push_back(Bidder(1, 50));

  va.AddSingleBid(bidders[0].funds, bidders[0].id, item);
  va.AddSingleBid(bidders[1].funds, bidders[1].id, item);

  std::size_t cur_block = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = va.Execute(cur_block);
    if (success)
    {
      execution_block = cur_block;

    }
    cur_block++;
  }

  std::cout << "Winner ID: " << va.Winner(item) << std::endl;
  std::cout << "Winner bid " << bidders[bidders.size() - 1].id << std::endl;
  std::cout << "Sale price" << va.Items()[0].sell_price << std::endl;

  ASSERT_TRUE(execution_block == end_block);
  ASSERT_TRUE(va.Winner(item) == bidders[0].id);
  ASSERT_TRUE(va.Items()[0].sell_price == bidders[1].funds);
}


TEST(vickrey_auction, many_bid_auction)
{

  std::size_t n_bidders = 10;
  std::size_t seller_id = 999;
  std::size_t start_block = 10000;
  std::size_t end_block = start_block + 9;
  std::size_t max_items = 1;

  // set up auction
  VickreyAuction va(start_block, end_block, max_items);

  // add item to auction
  ItemIdType item      = 0;
  ValueType  min_price = 7;
  va.AddItem(item, seller_id, min_price);

  // set up bidders
  std::vector<Bidder> bidders{};
  for (std::size_t i = 0; i < n_bidders; ++i)
  {
    bidders.push_back(Bidder(i, i));
  }

  // make bids
  for (std::size_t j = 0; j < n_bidders; ++j)
  {
    va.AddSingleBid(bidders[j].funds, bidders[j].id, item);
  }

  std::size_t cur_block = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = va.Execute(cur_block);
    if (success)
    {
      execution_block = cur_block;

    }
    cur_block++;
  }
  
  std::cout << "Winner ID: " << va.Winner(item) << std::endl;
  std::cout << "Winner bid " << bidders[bidders.size() - 1].id << std::endl;
  std::cout << "Sale price" << va.Items()[0].sell_price << std::endl;

  ASSERT_TRUE(execution_block == end_block);
  ASSERT_TRUE(va.Winner(item) == bidders[bidders.size() - 1].id);
  ASSERT_TRUE(va.Items()[0].sell_price == bidders[bidders.size() - 2].funds);
}
