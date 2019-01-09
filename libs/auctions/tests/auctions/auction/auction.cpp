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

#include <auctions/auction.hpp>


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

Auction SetupAuction(std::size_t start_block_val, std::size_t end_block_val)
{
  BlockIdType start_block(start_block_val);
  BlockIdType end_block(end_block_val);
  std::size_t max_items = 1;
  return Auction(start_block, end_block, max_items);
}

TEST(vickrey_auction, many_bid_auction)
{

  // set up auction
  std::size_t start_block = 10000;
  std::size_t end_block = 10010;
  Auction a = SetupAuction(start_block, end_block);

  // add item to auction
  ItemIdType item      = 0;
  ValueType  min_price = 7;
  std::size_t seller_id = 999;
  ErrorCode err = a.AddItem(item, seller_id, min_price);
  ASSERT_TRUE(err == ErrorCode::SUCCESS);


  std::size_t n_bidders = 10;

  // set up bidders
  std::vector<Bidder> bidders{};
  for (std::size_t i = 0; i < n_bidders; ++i)
  {
    bidders.push_back(Bidder(i, i));
  }

  // make bids
  for (std::size_t j = 0; j < n_bidders; ++j)
  {
    a.AddSingleBid(bidders[j].funds, bidders[j].id, item);
  }

  std::size_t cur_block = start_block;
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

  std::cout << "Winner ID: " << a.Winner(item) << std::endl;
  std::cout << "Winner bid " << bidders[bidders.size() - 1].id << std::endl;
  std::cout << "Sale price" << a.Items()[0].sell_price << std::endl;

  ASSERT_TRUE(execution_block == end_block);
  ASSERT_TRUE(a.Winner(item) == bidders[bidders.size() - 1].id);
  ASSERT_TRUE(a.Items()[0].sell_price == bidders[bidders.size() - 1].funds);
}
