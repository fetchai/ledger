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

VickreyAuction SetupAuction(std::size_t start_block_val, std::size_t end_block_val,
                            std::size_t max_items = 1)
{
  BlockIdType start_block(start_block_val);
  BlockIdType end_block(end_block_val);
  return VickreyAuction(start_block, end_block, max_items);
}

TEST(vickrey_auction, one_bid_auction)
{

  // set up auction
  std::size_t    start_block = 10000;
  std::size_t    end_block   = 10010;
  VickreyAuction va          = SetupAuction(start_block, end_block);

  // add item to auction
  ItemIdType  item      = 0;
  ValueType   min_price = 7;
  std::size_t seller_id = 999;
  ErrorCode   err       = va.AddItem(item, seller_id, min_price);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

  // set up bidders
  std::vector<Bidder> bidders{};
  bidders.push_back(Bidder(0, 100));

  va.AddSingleBid(bidders[0].funds, bidders[0].id, item);

  std::size_t cur_block_val   = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = va.Execute(BlockIdType(cur_block_val));
    if (success)
    {
      execution_block = cur_block_val;
    }
    cur_block_val++;
  }

  ASSERT_TRUE(execution_block == end_block);
  ASSERT_TRUE(va.Winner(item) == bidders[0].id);
  ASSERT_TRUE(va.Items()[0].sell_price == bidders[0].funds);
}

TEST(vickrey_auction, two_bid_auction)
{
  // set up auction
  std::size_t    start_block = 10000;
  std::size_t    end_block   = 10010;
  VickreyAuction va          = SetupAuction(start_block, end_block);

  // add item to auction
  ItemIdType  item      = 0;
  ValueType   min_price = 7;
  std::size_t seller_id = 999;
  ErrorCode   err       = va.AddItem(item, seller_id, min_price);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

  // set up bidders
  std::vector<Bidder> bidders{};
  bidders.push_back(Bidder(0, 100));
  bidders.push_back(Bidder(1, 50));

  va.AddSingleBid(bidders[0].funds, bidders[0].id, item);
  va.AddSingleBid(bidders[1].funds, bidders[1].id, item);

  std::size_t cur_block       = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = va.Execute(BlockIdType(cur_block));
    if (success)
    {
      execution_block = cur_block;
    }
    cur_block++;
  }

  ASSERT_TRUE(execution_block == end_block);
  ASSERT_TRUE(va.Winner(item) == bidders[0].id);
  ASSERT_TRUE(va.Items()[0].sell_price == bidders[1].funds);
}

TEST(vickrey_auction, many_bid_auction)
{
  // set up auction
  std::size_t    start_block = 10000;
  std::size_t    end_block   = 10010;
  VickreyAuction va          = SetupAuction(start_block, end_block);

  // add item to auction
  ItemIdType  item      = 0;
  ValueType   min_price = 7;
  std::size_t seller_id = 999;
  ErrorCode   err       = va.AddItem(item, seller_id, min_price);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

  // set up bidders
  std::size_t         n_bidders = 10;
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

  std::size_t cur_block       = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = va.Execute(BlockIdType(cur_block));
    if (success)
    {
      execution_block = cur_block;
    }
    cur_block++;
  }

  ASSERT_TRUE(execution_block == end_block);
  ASSERT_TRUE(va.Winner(item) == bidders[bidders.size() - 1].id);
  ASSERT_TRUE(va.Items()[0].sell_price == bidders[bidders.size() - 2].funds);
}

TEST(vickrey_auction, many_bid_many_item_auction)
{
  // set up auction
  std::size_t    start_block = 10000;
  std::size_t    end_block   = 10010;
  std::size_t    n_items     = 10;
  VickreyAuction va          = SetupAuction(start_block, end_block, n_items);

  // add item to auction
  for (std::size_t i = 0; i < n_items; ++i)
  {
    ItemIdType  item      = i;
    ValueType   min_price = 100 + i;
    std::size_t seller_id = 990 + i;
    ErrorCode   err       = va.AddItem(item, seller_id, min_price);
    ASSERT_TRUE(err == ErrorCode::SUCCESS);
  }

  // set up bidders
  std::size_t         n_bidders = 10;
  std::vector<Bidder> bidders{};
  for (std::size_t i = 0; i < n_bidders; ++i)
  {
    bidders.push_back(Bidder(i, 500 + (i * 10)));
  }

  // make bids
  for (std::size_t i = 0; i < n_items; ++i)
  {
    for (std::size_t j = 0; j < n_bidders; ++j)
    {
      va.AddSingleBid(bidders[j].funds / 10, bidders[j].id, i);
    }
  }

  std::size_t cur_block       = start_block;
  std::size_t execution_block = 0;
  for (std::size_t j = 0; j < 20; ++j)
  {
    bool success = va.Execute(BlockIdType(cur_block));
    if (success)
    {
      execution_block = cur_block;
    }
    cur_block++;
  }

  ItemsContainerType items = va.Items();
  for (std::size_t j = 0; j < n_items; ++j)
  {
    std::cout << "Winner ID: " << va.Winner(j) << std::endl;
    std::cout << "Winner bid " << bidders[bidders.size() - 1].id << std::endl;
    std::cout << "Sale price" << va.Items()[0].sell_price << std::endl;

    std::cout << "items[j].sell_price: " << items[j].sell_price << std::endl;
    std::cout << "bidders[bidders.size() - 2].funds: " << bidders[bidders.size() - 2].funds / 10
              << std::endl;

    ASSERT_TRUE(execution_block == end_block);
    ASSERT_TRUE(va.Winner(j) == bidders[bidders.size() - 1].id);
    ASSERT_TRUE(items[j].sell_price == bidders[bidders.size() - 2].funds / 10);
  }
}
