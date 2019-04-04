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

#include "math/tensor.hpp"
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

TEST(combinatorial_auction, many_bid_many_item_auction)
{

  ErrorCode err;

  // set up auction
  CombinatorialAuction ca = CombinatorialAuction();

  // add items to auction
  std::vector<Item> items{};

  AgentId seller_id = 990;
  Value   min_price = 0;
  ItemId  item_id   = 0;

  Item item1(item_id, seller_id, min_price);  // e.g. car
  items.push_back(item1);

  Item item2(item_id + 1, seller_id, min_price);
  items.push_back(item2);

  Item item3(item_id + 2, seller_id, min_price);
  items.push_back(item3);

  Item item4(item_id + 3, seller_id, min_price);
  items.push_back(item4);

  for (std::size_t k = 0; k < items.size(); ++k)
  {
    err = ca.AddItem(items[k]);
    ASSERT_EQ(err, ErrorCode::SUCCESS);
  }

  // set up bidders
  std::vector<Bidder> bidders{};
  bidders.emplace_back(Bidder(0, 500));

  // make bids

  BidId bid_id    = 0;
  Value bid_price = 10;
  Bid   bid1(bid_id, {items[0].id}, bid_price, bidders[0].id);
  err = ca.PlaceBid(bid1);
  ASSERT_EQ(err, ErrorCode::SUCCESS);

  bid_id    = 1;
  bid_price = 8;
  Bid bid2(bid_id, {items[1].id}, bid_price, bidders[0].id);
  err = ca.PlaceBid(bid2);
  ASSERT_EQ(err, ErrorCode::SUCCESS);

  bid_id    = 2;
  bid_price = 7;
  Bid bid3(bid_id, {items[2].id}, bid_price, bidders[0].id);
  err = ca.PlaceBid(bid3);
  ASSERT_EQ(err, ErrorCode::SUCCESS);

  bid_id    = 3;
  bid_price = 29;
  Bid bid4(bid_id, {items[0].id, items[1].id, items[2].id}, bid_price, bidders[0].id);
  err = ca.PlaceBid(bid4);
  ASSERT_EQ(err, ErrorCode::SUCCESS);

  bid_id    = 4;
  bid_price = 20;
  Bid bid5(bid_id, {items[0].id, items[1].id, items[2].id}, bid_price, bidders[0].id);

  bid_id    = 5;
  bid_price = 30;
  Bid bid6(bid_id, {items[0].id, items[1].id, items[3].id}, bid_price, bidders[0].id);

  bid5.excludes.emplace_back(bid6.id);
  bid6.excludes.emplace_back(bid5.id);

  err = ca.PlaceBid(bid5);
  ASSERT_EQ(err, ErrorCode::SUCCESS);
  err = ca.PlaceBid(bid6);
  ASSERT_EQ(err, ErrorCode::SUCCESS);

  bid_id    = 6;
  bid_price = 7;
  Bid bid7(bid_id, {items[3].id}, bid_price, bidders[0].id);
  err = ca.PlaceBid(bid7);
  ASSERT_EQ(err, ErrorCode::SUCCESS);

  for (std::size_t j = 0; j < 10; ++j)
  {
    ca.Mine(23 * j, 100);
    std::cout << "ca.TotalBenefit(): " << ca.TotalBenefit() << std::endl;
  }

  for (std::size_t j = 0; j < 7; ++j)
  {
    std::cout << "ca.Active(j): " << ca.Active(j) << std::endl;
  }

  // should accept one but not both of these bids since they're exclusive
  ASSERT_TRUE(((ca.Active(4) == 0) && (ca.Active(5) == 0)) || (ca.Active(4) != ca.Active(5)));
}
