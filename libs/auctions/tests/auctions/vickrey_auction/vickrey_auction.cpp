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

VickreyAuction SetupAuction(std::size_t start_block_val, std::size_t end_block_val)
{
  BlockIdType start_block(start_block_val);
  BlockIdType end_block(end_block_val);
  return VickreyAuction(start_block, end_block);
}

TEST(vickrey_auction, one_bid_auction)
{

  // set up auction
  std::size_t    start_block = 10000;
  std::size_t    end_block   = 10010;
  VickreyAuction va          = SetupAuction(start_block, end_block);

  // add item to auction
  ItemIdType  item_id   = 0;
  AgentIdType seller_id = 999;
  ValueType   min_price = 7;
  Item        item(item_id, seller_id, min_price);
  ErrorCode   err = va.AddItem(item);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

  // set up bidders
  std::vector<Bidder> bidders{};
  bidders.push_back(Bidder(0, 100));
  BidIdType bid_id = 0;
  Bid       cur_bid(bid_id, {item}, bidders[0].funds, bidders[0].id);
  err = va.PlaceBid(cur_bid);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

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
  ASSERT_TRUE(va.Winner(item.Id()) == bidders[0].id);
  ASSERT_TRUE(va.Items()[0].SellPrice() == bidders[0].funds);
}

TEST(vickrey_auction, two_bid_auction)
{
  // set up auction
  std::size_t    start_block = 10000;
  std::size_t    end_block   = 10010;
  VickreyAuction va          = SetupAuction(start_block, end_block);

  // add item to auction
  ItemIdType  item_id   = 0;
  AgentIdType seller_id = 999;
  ValueType   min_price = 7;
  Item        item(item_id, seller_id, min_price);
  ErrorCode   err = va.AddItem(item);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

  // set up bidders
  std::vector<Bidder> bidders{};
  bidders.push_back(Bidder(0, 100));
  bidders.push_back(Bidder(1, 50));

  BidIdType bid_id = 0;
  Bid       bid1(bid_id, {item}, bidders[0].funds, bidders[0].id);
  err = va.PlaceBid(bid1);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

  bid_id = 1;
  Bid bid2(bid_id, {item}, bidders[1].funds, bidders[1].id);
  err = va.PlaceBid(bid2);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);

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
  ASSERT_TRUE(va.Winner(item.Id()) == bidders[0].id);
  ASSERT_TRUE(va.Items()[0].SellPrice() == bidders[1].funds);
}

TEST(vickrey_auction, many_bid_auction)
{
  // set up auction
  std::size_t    start_block = 10000;
  std::size_t    end_block   = 10010;
  VickreyAuction va          = SetupAuction(start_block, end_block);

  // add item to auction
  ItemIdType  item_id   = 0;
  AgentIdType seller_id = 999;
  ValueType   min_price = 7;
  Item        item(item_id, seller_id, min_price);
  ErrorCode   err = va.AddItem(item);
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
    bid_id = j;
    Bid cur_bid(bid_id, {item}, bidders[j].funds, bidders[j].id);
    err = va.PlaceBid(cur_bid);
    ASSERT_TRUE(err == ErrorCode::SUCCESS);
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
  ASSERT_TRUE(va.Winner(item.Id()) == bidders[bidders.size() - 1].id);
  ASSERT_TRUE(va.Items()[0].SellPrice() == bidders[bidders.size() - 2].funds);
}

TEST(vickrey_auction, many_bid_many_item_auction)
{
  ErrorCode err;

  // set up auction
  std::size_t    start_block = 10000;
  std::size_t    end_block   = 10010;
  std::size_t    n_items     = 10;
  VickreyAuction va          = SetupAuction(start_block, end_block);

  // add item to auction
  std::vector<Item> items{};
  ItemIdType        item_id;
  AgentIdType       seller_id;
  ValueType         min_price;
  for (std::size_t i = 0; i < n_items; ++i)
  {
    item_id   = i;
    seller_id = 990 + i;
    min_price = 100 + i;
    Item item(item_id, seller_id, min_price);
    items.emplace_back(item);
    err = va.AddItem(item);
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
  std::size_t bid_count = 0;
  BidIdType   bid_id;
  for (std::size_t i = 0; i < n_items; ++i)
  {
    for (std::size_t j = 0; j < n_bidders; ++j)
    {
      bid_id = bid_count;
      Bid cur_bid(bid_id, {items[i]}, bidders[j].funds / 10, bidders[j].id);
      err = va.PlaceBid(cur_bid);
      ASSERT_TRUE(err == ErrorCode::SUCCESS);
      bid_count += 1;
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

  for (std::size_t j = 0; j < n_items; ++j)
  {
    ASSERT_TRUE(execution_block == end_block);
    ASSERT_TRUE(va.Winner(j) == bidders[bidders.size() - 1].id);
    ASSERT_TRUE(va.Items()[j].SellPrice() == bidders[bidders.size() - 2].funds / 10);
  }
}
