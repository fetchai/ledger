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

  Bidder(std::size_t funds, std::size_t id)
    : id(id)
    , funds(funds)
  {
    ;
  }

private:
  Bidder() = default;
};

TEST(vickrey_auction, many_bid_auction)
{

  std::size_t n_bidders = 10;
  std::size_t seller_id = 999;
  std::size_t start_block = 10000;
  std::size_t end_block = start_block + 9;
  std::size_t max_items = 1;

  // set up auction
  Auction a(start_block, end_block, max_items);

  // add item to auction
  ItemIdType item      = 0;
  ValueType  min_price = 7;
  a.AddItem(item, seller_id, min_price);

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
  for (std::size_t j = 0; j < 20; ++j)
  {
    std::cout << "cur_block: " << cur_block << std::endl;
    bool success = a.Execute(cur_block);
    std::cout << "a.Winner(item): " << a.Winner(item) << std::endl;
    std::cout << "success: " << success << std::endl;

    cur_block++;
  }

  std::cout << "a.Winner(item): " << a.Winner(item) << std::endl;
  std::cout << "bidders[bidders.size() - 1].id: " << bidders[bidders.size() - 1].id << std::endl;
  std::cout << "a.Items()[0].sell_price: " << a.Items()[0].sell_price << std::endl;

  ASSERT_TRUE(a.Winner(item) == bidders[bidders.size() - 1].id);
}
